#include "RARC.hpp"
#include <core/common.h>
#include <core/util/oishii.hpp>
#include <fstream>
#include <iostream>
#include <librii/szs/SZS.hpp>
#include <rsl/SimpleReader.hpp>

static u16 calc_key_code(const std::string& key) {
  u32 code = 0;
  for (auto& c : key) {
    code = c + code * 3;
  }
  return code;
}

namespace librii::RARC {

struct rarcMetaHeader {
  rsl::bu32 magic;
  rsl::bu32 size;
  struct {
    rsl::bu32 offset;
  } nodes;
  struct {
    rsl::bu32 offset;
    rsl::bu32 size;
    rsl::bu32 mram_size;
    rsl::bu32 aram_size;
    rsl::bu32 dvd_size;
  } files;
};

static_assert(sizeof(rarcMetaHeader) == 32);

struct rarcNodesHeader {
  struct {
    rsl::bu32 count;
    rsl::bu32 offset;
  } dir_nodes;
  struct {
    rsl::bu32 count;
    rsl::bu32 offset;
  } fs_nodes;
  rsl::bu32 string_table_size;
  rsl::bu32 strings_offset;
  rsl::bu16 id_max;
  bool ids_synced;
  u8 padding[5];
};

static_assert(sizeof(rarcNodesHeader) == 32);

struct rarcDirectoryNode {
  rsl::bu32 magic;
  rsl::bu32 name;
  rsl::bu16 hash;
  rsl::bu16 child_count;
  rsl::bu32 children_offset;
};

static_assert(sizeof(rarcDirectoryNode) == 16);

struct rarcFSNode {
  rsl::bu16 id;
  rsl::bu16 hash;
  rsl::bu16 type;
  rsl::bu16 name;
  union {
    struct {
      rsl::bs32 dir_node;
      rsl::bu32 size; // Always 0x10
    } folder;
    struct {
      rsl::bu32 offset;
      rsl::bu32 size;
    } file;
  };
  rsl::bu32 padding;
};

static_assert(sizeof(rarcFSNode) == 20);

struct LowResourceArchive {
  rarcMetaHeader header;
  std::vector<rarcDirectoryNode> dir_nodes;
  std::vector<rarcFSNode> fs_nodes;
  std::string strings;       // One giant string
  std::vector<u8> file_data; // One giant buffer
};

static inline const rarcNodesHeader*
rarcGetNodesHeader(const rarcMetaHeader* self) {
  assert(self->nodes.offset >= sizeof(rarcMetaHeader));
  return (const rarcNodesHeader*)(((u8*)self) + self->nodes.offset);
}

static inline const rarcDirectoryNode*
rarcGetDirectoryNodes(const rarcNodesHeader* self) {
  assert(self->dir_nodes.offset >= sizeof(rarcNodesHeader));
  return (const rarcDirectoryNode*)(((u8*)self) + self->dir_nodes.offset);
}

static inline const rarcFSNode* rarcGetFSNodes(const rarcNodesHeader* self) {
  assert(self->fs_nodes.offset >=
         self->dir_nodes.offset +
             roundUp(sizeof(rarcDirectoryNode) * self->dir_nodes.count, 32));
  return (const rarcFSNode*)(((u8*)self) + self->fs_nodes.offset);
}

static inline const char* rarcGetFSNames(const rarcNodesHeader* self) {
  assert(self->strings_offset >=
         self->fs_nodes.offset +
             roundUp(sizeof(rarcFSNode) * self->fs_nodes.count, 32));
  return ((const char*)self) + self->strings_offset;
}

static inline const u8* rarcGetFileData(const rarcMetaHeader* self) {
  auto* node_info = rarcGetNodesHeader(self);
  assert(self->files.offset >=
         node_info->strings_offset + node_info->string_table_size);
  return (const u8*)((((u8*)self) +
                      roundUp(self->nodes.offset + self->files.offset, 32)));
};

bool rarcNodeIsFolder(const rarcFSNode self) {
  return self.type & (ResourceAttribute::DIRECTORY << 8);
}

template <typename T> bool SafeMemCopy(T& dest, rsl::byte_view data) {
  if (data.size_bytes() < sizeof(T)) {
    std::fill((u8*)&dest, (u8*)(&dest + 1), 0);
    return false;
  }

  std::memcpy(&dest, data.data(), sizeof(T));
  return true;
}

bool RangeContains(rsl::byte_view range, const void* ptr) {
  return ptr >= range.data() && ptr < (range.data() + range.size());
}

bool RangeContainsInclusive(rsl::byte_view range, const void* ptr) {
  return ptr >= range.data() && ptr <= range.data() + range.size();
}

Result<void> LoadResourceArchiveLow(LowResourceArchive& result,
                                    rsl::byte_view data) {
  if (!SafeMemCopy(result.header, data))
    return std::unexpected("Incomplete meta header data!");

  const auto* node_info =
      rarcGetNodesHeader(reinterpret_cast<const rarcMetaHeader*>(data.data()));
  if (!RangeContains(data, node_info))
    return std::unexpected("Incomplete node header data!");

  const auto* dir_nodes_begin = rarcGetDirectoryNodes(node_info);
  const auto* dir_nodes_end = dir_nodes_begin + node_info->dir_nodes.count;

  if (!RangeContainsInclusive(data, dir_nodes_end))
    return std::unexpected("Incomplete directory node table!");

  result.dir_nodes = {dir_nodes_begin, dir_nodes_end};

  const auto* fs_nodes_begin = rarcGetFSNodes(node_info);
  const auto* fs_nodes_end = fs_nodes_begin + node_info->fs_nodes.count;

  if (!RangeContainsInclusive(data, fs_nodes_end))
    return std::unexpected("Incomplete filesystem node table!");

  result.fs_nodes = {fs_nodes_begin, fs_nodes_end};

  const char* strings_begin = rarcGetFSNames(node_info);
  if (!RangeContains(data, strings_begin))
    return std::unexpected("String table has invalid offset!");

  const char* strings_end =
      reinterpret_cast<const char*>(node_info) + result.header.files.offset;
  if (!RangeContainsInclusive(data, strings_end))
    return std::unexpected("Incomplete string table!");

  result.strings = {strings_begin, strings_end};

  auto* fd_begin =
      rarcGetFileData(reinterpret_cast<const rarcMetaHeader*>(data.data()));
  if (!RangeContains(data, fd_begin))
    return std::unexpected("Resource data has invalid offset");

  result.file_data = {fd_begin, data.data() + data.size()};

  return {};
}

static void rarcRecurseLoadDirectory(
    ResourceArchive& arc, LowResourceArchive& low, rarcDirectoryNode& low_dir,
    std::optional<rarcFSNode> low_node,
    std::optional<ResourceArchive::Node> dir_parent,
    std::vector<ResourceArchive::Node>& out, std::size_t depth) {
  const auto start_nodes = out.size();

  ResourceArchive::Node dir_node;
  if (low_node) {
    dir_node = {.id = static_cast<s32>(low_node->folder.dir_node),
                .flags = static_cast<u16>(low_node->type >> 8),
                .name = low.strings.data() + low_dir.name};
  } else {
    // This is the ROOT directory (exists without an fs node to further
    // describe it)
    dir_node = {.id = 0,
                .flags = ResourceAttribute::DIRECTORY,
                .name = low.strings.data() + low_dir.name};
  }

  dir_node.folder.parent = dir_parent ? dir_parent->id : -1;

  auto* fs_begin = low.fs_nodes.data() + (low_dir.children_offset);
  auto* fs_end =
      low.fs_nodes.data() + (low_dir.children_offset + low_dir.child_count);

  // dir_node.folder.sibling_next = fs_end->id;

  ResourceArchive::Node dir_ref_node;
  ResourceArchive::Node dir_parent_ref_node;

  for (auto* fs_node = fs_begin; fs_node != fs_end; ++fs_node) {
    if (rarcNodeIsFolder(*fs_node)) {
      if ((low_node && fs_node->folder.dir_node == low_node->folder.dir_node) ||
          (!low_node && fs_node->folder.dir_node == 0)) {
        // Path "."
        dir_ref_node = {.id = fs_node->folder.dir_node,
                        .flags = static_cast<u16>(fs_node->type >> 8),
                        .name = low.strings.data() + fs_node->name};
        continue;
      }

      if ((dir_parent && fs_node->folder.dir_node == dir_parent->id) ||
          fs_node->folder.dir_node == -1) {
        // Path ".."
        dir_parent_ref_node = {.id = dir_parent ? dir_parent->id : -1,
                               .flags = static_cast<u16>(fs_node->type >> 8),
                               .name = low.strings.data() + fs_node->name};
        continue;
      }

      // Sub Directory
      auto& chlld_dir_n = low.dir_nodes.at(fs_node->folder.dir_node);
      rarcRecurseLoadDirectory(arc, low, chlld_dir_n, *fs_node, dir_node, out,
                               depth + 1);
    } else {
      ResourceArchive::Node tmp_f = {
          .id = fs_node->id,
          .flags = static_cast<u16>(fs_node->type >> 8),
          .name = low.strings.data() + fs_node->name};

      auto data_begin = low.file_data.begin() + fs_node->file.offset;
      tmp_f.data.insert(tmp_f.data.end(), data_begin,
                        data_begin + fs_node->file.size);
      out.push_back(tmp_f);
    }
  }

  dir_parent_ref_node.folder.parent =
      dir_parent ? dir_parent->folder.parent : -1;
  dir_parent_ref_node.folder.sibling_next =
      dir_parent ? dir_parent->folder.sibling_next : 0;
  out.insert(out.begin() + start_nodes, dir_parent_ref_node);

  dir_ref_node.folder.parent = dir_node.folder.parent;
  dir_ref_node.folder.sibling_next = dir_node.folder.sibling_next;
  out.insert(out.begin() + start_nodes, dir_ref_node);

  dir_node.folder.sibling_next = out.size() + depth * 3 + 1;
  out.insert(out.begin() + start_nodes, dir_node);
}

bool IsDataResourceArchive(rsl::byte_view data) {
  return data[0] == 'R' && data[1] == 'A' && data[2] == 'R' && data[3] == 'C';
}

Result<ResourceArchive> LoadResourceArchive(rsl::byte_view data) {
  ResourceArchive result;
  LowResourceArchive low;

  auto low_result = LoadResourceArchiveLow(low, data);
  if (!low_result)
    return std::unexpected(low_result.error());

  rarcRecurseLoadDirectory(result, low, low.dir_nodes[0], std::nullopt,
                           std::nullopt, result.nodes, 0);
  return result;
}

using children_info_type =
    std::unordered_map<std::string, std::pair<std::size_t, std::size_t>>;

std::vector<ResourceArchive::Node>
rarcSortFSNodesForSave(const ResourceArchive& arc,
                       children_info_type& children_info);

Result<std::vector<u8>> SaveResourceArchive(const ResourceArchive& arc,
                                            bool make_matching,
                                            bool ids_synced) {
  struct OffsetInfo {
    std::size_t string_offset;
    std::size_t fs_node_offset;
  };
  std::unordered_map<std::string, OffsetInfo> offsets_map;

  // Used for generated fs_node and name data, the dir table is DFS order
  // but relies on this data too, hence children_infos
  children_info_type children_infos;
  auto fs_sorted_nodes = rarcSortFSNodesForSave(arc, children_infos);

  std::string strings_blob;
  {
    offsets_map["."] = {0, 0};
    offsets_map[".."] = {2, 0};
    strings_blob += "." + std::string("\0", 1) + ".." + std::string("\0", 1);
    std::size_t fs_info_offset = 0;
    for (auto& node : arc.nodes) {
      if (make_matching) {
        if (!node.is_special_path()) {
          offsets_map[node.name + std::to_string(node.id)] = {
              strings_blob.size(), fs_info_offset};
          strings_blob += node.name + std::string("\0", 1);
        }
      } else {
        if (!offsets_map.contains(node.name)) {
          offsets_map[node.name] = {strings_blob.size(), fs_info_offset};
          strings_blob += node.name + std::string("\0", 1);
        }
      }
      fs_info_offset += sizeof(rarcFSNode);
    }
  }

  std::vector<rarcDirectoryNode> dir_nodes;
  std::vector<rarcFSNode> fs_nodes;

  std::size_t mram_size = 0, aram_size = 0, dvd_size = 0;
  std::size_t current_child_offset = 0;

  // Directories follow DFS
  for (auto& node : arc.nodes) {
    OffsetInfo offsets = offsets_map[make_matching && !node.is_special_path()
                                         ? node.name + std::to_string(node.id)
                                         : node.name];

    if (!node.is_folder())
      continue;

    // Special paths don't have directory entries
    const bool is_special_path = node.is_special_path();
    if (!is_special_path) {
      rarcDirectoryNode low_dir;

      std::string s_magic = node.name;
      std::transform(s_magic.begin(), s_magic.end(), s_magic.begin(),
                     ::toupper);
      if (s_magic.size() < 4)
        s_magic.append(4 - s_magic.size(), ' ');

      low_dir.name = offsets.string_offset;
      low_dir.hash = calc_key_code(node.name);

      const auto& info = children_infos[node.name + std::to_string(node.id)];
      low_dir.child_count = info.first;
      low_dir.children_offset = current_child_offset;

      current_child_offset += low_dir.child_count;

      if (node.folder.parent != -1) {
        low_dir.magic = *reinterpret_cast<rsl::bu32*>(s_magic.data());
      } else {
        low_dir.magic = static_cast<rsl::bu32>('ROOT');
      }

      dir_nodes.push_back(low_dir);
    }
  }

  // The FS node table however does not follow DFS :P

  // We store a vector of each used data offset to know if we need
  // to actually increment the data size marker. This is because
  // many files can point to the same piece of data to save space
  std::vector<u8> low_data;
  std::vector<std::size_t> used_offsets;
  for (auto& node : fs_sorted_nodes) {
    OffsetInfo offsets = offsets_map[make_matching && !node.is_special_path()
                                         ? node.name + std::to_string(node.id)
                                         : node.name];

    rarcFSNode low_node = {.hash = calc_key_code(node.name),
                           .type = node.flags << 8,
                           .name = offsets.string_offset};

    if (node.is_folder()) {
      // Skip the root.
      if (!node.is_special_path() && node.folder.parent == -1) {
        continue;
      }

      low_node.id = 0xFFFF;
      low_node.folder.dir_node = node.id;
      low_node.folder.size = 0x10;

      fs_nodes.push_back(low_node);
    } else {
      // IDs are calculated by incrementing for each file and special dir
      // The difference of FS nodes and pure Dir nodes is this total
      low_node.id = node.id;
      low_node.file.offset = low_data.size();
      low_node.file.size = node.data.size();

      const bool is_shared_data = std::any_of(
          fs_sorted_nodes.begin(), fs_sorted_nodes.end(), [&](auto& n) {
            if (n.id == node.id)
              return false;
            return n.data.size() == node.data.size() &&
                   std::equal(n.data.begin(), n.data.end(), node.data.begin());
          });

      // We can optimize RARCs by condensing redundant data and sharing offsets
      // However it doesn't seem like Nintendo does this so we should
      // opt to store all data anyway when trying to match 1:1
      if (!is_shared_data || make_matching) {
        std::size_t padded_size = roundUp(node.data.size(), 32);
        if ((node.flags & ResourceAttribute::PRELOAD_TO_MRAM)) {
          mram_size += padded_size;
        } else if ((node.flags & ResourceAttribute::PRELOAD_TO_ARAM)) {
          aram_size += padded_size;
        } else if ((node.flags & ResourceAttribute::LOAD_FROM_DVD)) {
          dvd_size += padded_size;
        } else {
          return std::unexpected(std::format(
              "File \"{}\" hasn't been marked for loading!", node.name));
        }
        low_data.insert(low_data.end(), node.data.begin(), node.data.end());
        low_data.insert(low_data.end(), padded_size - node.data.size(), '\0');
      }
      fs_nodes.push_back(low_node);
    }
  }

  std::size_t total_file_size = mram_size + aram_size + dvd_size;
  if (total_file_size != low_data.size())
    return std::unexpected(
        "Marked total file size doesn't match true data size!");

  // Node metadata
  // TODO: Verify custom unsynced IDs?
  rarcNodesHeader node_header{.id_max = fs_nodes.size(),
                              .ids_synced = ids_synced};
  node_header.dir_nodes.count = dir_nodes.size();
  node_header.dir_nodes.offset = sizeof(rarcNodesHeader);
  node_header.fs_nodes.count = fs_nodes.size();
  node_header.fs_nodes.offset =
      node_header.dir_nodes.offset +
      roundUp(node_header.dir_nodes.count * sizeof(rarcDirectoryNode), 32);
  node_header.string_table_size = roundUp(strings_blob.size(), 32);
  node_header.strings_offset =
      node_header.fs_nodes.offset +
      roundUp(node_header.fs_nodes.count * sizeof(rarcFSNode), 32);

  // Calculate header connections, data info, and size information
  rarcMetaHeader meta_header{
      .magic = static_cast<rsl::bu32>('RARC')}; // Little endian 'RARC'
  meta_header.nodes.offset = sizeof(rarcMetaHeader);
  meta_header.files.offset =
      roundUp(node_header.strings_offset + strings_blob.size(), 32);
  meta_header.files.size = total_file_size;
  meta_header.files.mram_size = mram_size;
  meta_header.files.aram_size = aram_size;
  meta_header.files.dvd_size = dvd_size;
  meta_header.size = meta_header.nodes.offset + meta_header.files.offset +
                     meta_header.files.size;

  std::vector<u8> result(meta_header.size);

  memcpy(&result[0], &meta_header, sizeof(rarcMetaHeader));
  memcpy(&result[meta_header.nodes.offset], &node_header,
         sizeof(rarcNodesHeader));
  memcpy(&result[meta_header.nodes.offset + node_header.dir_nodes.offset],
         dir_nodes.data(),
         sizeof(rarcDirectoryNode) * node_header.dir_nodes.count);
  memcpy(&result[meta_header.nodes.offset + node_header.fs_nodes.offset],
         fs_nodes.data(), sizeof(rarcFSNode) * node_header.fs_nodes.count);
  memcpy(&result[meta_header.nodes.offset + node_header.strings_offset],
         strings_blob.data(), strings_blob.size());
  memcpy(&result[meta_header.nodes.offset + meta_header.files.offset],
         low_data.data(), low_data.size());

  if (result.size() != meta_header.size)
    return std::unexpected(
        "Saved data mismatched the recorded data size provided!");

  return result;
}

void RecalculateArchiveIDs(
    ResourceArchive& arc, const std::vector<ResourceArchive::Node>& calc_list) {
  assert(arc.nodes.size() == calc_list.size());

  s16 id = -1; // Adjust for root node
  for (auto& node : calc_list) {
    if (node.is_folder()) {
      id++;
      continue;
    }

    for (auto& target : arc.nodes) {
      if (target == node) {
        target.id = id++;
        break;
      }
    }
  }
}

Result<void> Extract(const ResourceArchive& arc, std::filesystem::path out) {
  auto tmp = out;
  std::vector<u32> stack;
  u32 i = 0;
  for (auto& node : arc.nodes) {
    if (!stack.empty()) {
      while (stack.back() == i) {
        stack.resize(stack.size() - 1);
        tmp = tmp.parent_path();
      }
    }
    ++i;
    if (node.is_special_path())
      continue;
    if (node.is_folder()) {
      stack.push_back(node.folder.sibling_next);
      tmp /= node.name;
      std::filesystem::create_directory(tmp);
    } else {
      auto fpath = (tmp / node.name).string();
      std::ofstream fout(fpath, std::ios::binary | std::ios::ate);
      fout.write(reinterpret_cast<const char*>(node.data.data()),
                 node.data.size());
    }
  }
  return {};
}

void get_sorted_directory_list_r(const std::filesystem::path& path,
                                 std::vector<std::filesystem::path>& out) {
  std::vector<std::filesystem::path> dirs;
  for (auto&& it : std::filesystem::directory_iterator{path}) {
    if (it.is_directory())
      dirs.push_back(it.path());
    else
      out.push_back(it.path());
  }
  for (auto& dir : dirs) {
    out.push_back(dir);
    get_sorted_directory_list_r(dir, out);
  }
}

Result<ResourceArchive> Create(std::filesystem::path root) {
  struct Entry {
    std::string str;
    bool is_folder = false;

    std::vector<u8> data;
    std::string name;

    int depth = -1;
    int parent = 0; // Default files put parent as 0 for root
    int siblingNext = -1;

    bool operator<(const Entry& rhs) const {
      return str < rhs.str && (is_folder ? 1 : 0) < (rhs.is_folder ? 1 : 0);
    }
  };
  std::vector<Entry> paths;

  paths.push_back(
      Entry{.str = (std::filesystem::path(".") / root.filename()).string(),
            .is_folder = true,
            .name = root.filename().string(),
            .parent = -1});
  paths.push_back(Entry{
      .str = (std::filesystem::path(".") / root.filename() / ".").string(),
      .is_folder = true,
      .name = "."});
  paths.push_back(Entry{
      .str = (std::filesystem::path(".") / root.filename() / "..").string(),
      .is_folder = true,
      .name = ".."});

  std::vector<std::filesystem::path> sorted_fs_tree;
  get_sorted_directory_list_r(root, sorted_fs_tree);

  for (auto& path : sorted_fs_tree) {
    bool folder = std::filesystem::is_directory(path);
    if (path.filename() == ".DS_Store") {
      continue;
    }
    std::vector<u8> data;
    if (!folder) {
      auto f = ReadFile(path.string());
      if (!f) {
        return std::unexpected(
            std::format("Failed to read file {}", path.string()));
      }
      data = *f;
    }
    path = std::filesystem::path(".") /
           std::filesystem::relative(path, root.parent_path());

    paths.push_back(Entry{
        .str = path.string(),
        .is_folder = folder,
        .data = data,
        .name = path.filename().string(),
    });

    if (folder) {
      paths.push_back(
          Entry{.str = (path / ".").string(), .is_folder = true, .name = "."});
      paths.push_back(Entry{
          .str = (path / "..").string(), .is_folder = true, .name = ".."});
    }
  }
  // They are almost certainly already in sorted order
  for (auto& p : paths) {
    for (auto c : p.str) {
      // relative paths are weakly_canonical
      if (c == '/' || c == '\\') {
        ++p.depth;
      }
    }
  }
  paths[0].siblingNext = paths.size();
  for (size_t i = 1; i < paths.size(); ++i) {
    auto& p = paths[i];
    if (p.is_folder) {
      for (int j = i + 1; j < paths.size(); ++j) {
        if (paths[j].depth <= p.depth) {
          p.siblingNext = j;
          break;
        }
        if (paths[j].depth == p.depth + 1)
          paths[j].parent = i;
      }
      if (p.siblingNext == -1)
        p.siblingNext = paths.size();
    }
  }
  for (auto& p : paths) {
    fmt::print("PATH: {} (folder:{}, depth:{})\n", p.str, p.is_folder, p.depth);
  }
  ResourceArchive result;

  s16 file_id = 0, folder_id = 0;
  std::vector<ResourceArchive::Node> parent_stack;
  for (auto& p : paths) {
    ResourceArchive::Node node{.flags = 0};
    while (p.depth < parent_stack.size() && parent_stack.size() > 0) {
      parent_stack.pop_back();
    }
    if (p.is_folder) {
      if (p.name == ".") {
        node = parent_stack[p.depth - 1];
        file_id++;
      } else if (p.name == "..") {
        if (parent_stack.size() > 1) {
          node = parent_stack[p.depth - 2];
        } else {
          // In this case the parent of the root is null
          node.id = -1;
          node.folder.parent = -1;
          node.folder.sibling_next = -1;
        }
        file_id++;
      } else {
        node.id = folder_id++;
        node.folder.parent =
            parent_stack.size() > 0 ? parent_stack[p.depth - 1].id : -1;
        node.folder.sibling_next = p.siblingNext;
        if (p.depth >= parent_stack.size()) {
          parent_stack.push_back(node);
        }
      }
      node.flags |= ResourceAttribute::DIRECTORY;
    } else {
      node.id = file_id++;
      node.flags |= ResourceAttribute::FILE;
      node.flags |= ResourceAttribute::PRELOAD_TO_MRAM;

      // TODO: Take YAY0 into account when it becomes supported
      if (librii::szs::isDataYaz0Compressed(
              rsl::byte_view(p.data.data(), p.data.size()))) {
        node.flags |= ResourceAttribute::COMPRESSED;
        node.flags |= ResourceAttribute::YAZ0_COMPRESSED;
      }

      node.data = p.data;
    }
    node.name = p.name;
    result.nodes.push_back(node);
  }

  children_info_type throwaway;
  RecalculateArchiveIDs(result, rarcSortFSNodesForSave(result, throwaway));
  return result;
}

void rarcSortFSNodeForSaveRecursive(const ResourceArchive& arc,
                                    std::vector<ResourceArchive::Node>& out,
                                    children_info_type& children_info,
                                    std::size_t node_index) {
  if (node_index > arc.nodes.size() - 2) // Account for the children after
    return;

  const auto& node = arc.nodes.at(node_index);

  assert(node.folder.sibling_next <= arc.nodes.size()); // Account for end

  std::vector<ResourceArchive::Node> contents;
  std::vector<ResourceArchive::Node> dirs, sub_nodes, files, special_dirs;

  // Now push all children of this folder into their respective vectors
  auto begin = arc.nodes.begin() + node_index + 1;
  auto end = arc.nodes.begin() + node.folder.sibling_next;

  for (auto& child_node = begin; child_node != end; ++child_node) {
    if (child_node->is_folder()) {
      if (child_node->is_special_path()) {
        special_dirs.push_back(*child_node);
      } else {
        dirs.push_back(*child_node);
        std::vector<ResourceArchive::Node> nodes;
        rarcSortFSNodeForSaveRecursive(
            arc, nodes, children_info,
            std::distance(arc.nodes.begin(), child_node));
        sub_nodes.insert(sub_nodes.end(), nodes.begin(), nodes.end());
        child_node += nodes.size();
      }
    } else {
      files.push_back(*child_node);
    }
  }

  size_t surface_children_size =
      files.size() + dirs.size() + special_dirs.size();
  children_info[node.name + std::to_string(node.id)] = {
      surface_children_size, node_index + surface_children_size + 1};

  // Apply items in the order that Nintendo's tools did
  out.insert(out.end(), files.begin(), files.end());
  out.insert(out.end(), dirs.begin(), dirs.end());
  out.insert(out.end(), special_dirs.begin(), special_dirs.end());
  out.insert(out.end(), sub_nodes.begin(), sub_nodes.end());
}

std::vector<ResourceArchive::Node>
rarcSortFSNodesForSave(const ResourceArchive& arc,
                       children_info_type& children_info) {
  std::vector<ResourceArchive::Node> out;

  // Iterate over all nodes. We start with those that have no parent (root
  // nodes).
  assert(arc.nodes.size() > 0 && arc.nodes[0].folder.parent == -1);
  out.push_back(arc.nodes[0]);
  rarcSortFSNodeForSaveRecursive(arc, out, children_info, 0);
  return out;
}

} // namespace librii::RARC
