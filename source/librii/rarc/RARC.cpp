#include "RARC.hpp"
#include <core/common.h>
#include <core/util/oishii.hpp>
#include <fstream>
#include <iostream>
#include <librii/szs/SZS.hpp>
#include <rsl/SimpleReader.hpp>

// INTERNAL //

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

static inline Result<const rarcNodesHeader*>
rarcGetNodesHeader(const rarcMetaHeader* self) {
  EXPECT(self->nodes.offset >= sizeof(rarcMetaHeader));
  return (const rarcNodesHeader*)(((u8*)self) + self->nodes.offset);
}

static inline Result<const rarcDirectoryNode*>
rarcGetDirectoryNodes(const rarcNodesHeader* self) {
  EXPECT(self->dir_nodes.offset >= sizeof(rarcNodesHeader));
  return (const rarcDirectoryNode*)(((u8*)self) + self->dir_nodes.offset);
}

static inline Result<const rarcFSNode*>
rarcGetFSNodes(const rarcNodesHeader* self) {
  EXPECT(self->fs_nodes.offset >=
         self->dir_nodes.offset +
             roundUp(sizeof(rarcDirectoryNode) * self->dir_nodes.count, 32));
  return (const rarcFSNode*)(((u8*)self) + self->fs_nodes.offset);
}

static inline Result<const char*> rarcGetFSNames(const rarcNodesHeader* self) {
  EXPECT(self->strings_offset >=
         self->fs_nodes.offset +
             roundUp(sizeof(rarcFSNode) * self->fs_nodes.count, 32));
  return ((const char*)self) + self->strings_offset;
}

static inline Result<const u8*> rarcGetFileData(const rarcMetaHeader* self) {
  auto* node_info = TRY(rarcGetNodesHeader(self));
  EXPECT(self->files.offset >=
         node_info->strings_offset + node_info->string_table_size);
  return (const u8*)((((u8*)self) +
                      roundUp(self->nodes.offset + self->files.offset, 32)));
};

static bool rarcNodeIsFolder(const rarcFSNode self) {
  return self.type & (ResourceAttribute::DIRECTORY << 8);
}

static bool rarcIsSpecialPath(const std::string& name) {
  return name == "." || name == "..";
}

template <typename T> static bool SafeMemCopy(T& dest, rsl::byte_view data) {
  if (data.size_bytes() < sizeof(T)) {
    std::fill((u8*)&dest, (u8*)(&dest + 1), 0);
    return false;
  }

  std::memcpy(&dest, data.data(), sizeof(T));
  return true;
}

static bool RangeContains(rsl::byte_view range, const void* ptr) {
  return ptr >= range.data() && ptr < (range.data() + range.size());
}

static bool RangeContainsInclusive(rsl::byte_view range, const void* ptr) {
  return ptr >= range.data() && ptr <= range.data() + range.size();
}

static void GetSortedDirectoryListR(const std::filesystem::path& path,
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
    GetSortedDirectoryListR(dir, out);
  }
}

Result<void> LoadResourceArchiveLow(LowResourceArchive& result,
                                    rsl::byte_view data) {
  if (!SafeMemCopy(result.header, data))
    return std::unexpected("Incomplete meta header data!");

  const auto* node_info = TRY(
      rarcGetNodesHeader(reinterpret_cast<const rarcMetaHeader*>(data.data())));
  if (!RangeContains(data, node_info))
    return std::unexpected("Incomplete node header data!");

  const auto* dir_nodes_begin = TRY(rarcGetDirectoryNodes(node_info));
  const auto* dir_nodes_end = dir_nodes_begin + node_info->dir_nodes.count;

  if (!RangeContainsInclusive(data, dir_nodes_end))
    return std::unexpected("Incomplete directory node table!");

  result.dir_nodes = {dir_nodes_begin, dir_nodes_end};

  const auto* fs_nodes_begin = TRY(rarcGetFSNodes(node_info));
  const auto* fs_nodes_end = fs_nodes_begin + node_info->fs_nodes.count;

  if (!RangeContainsInclusive(data, fs_nodes_end))
    return std::unexpected("Incomplete filesystem node table!");

  result.fs_nodes = {fs_nodes_begin, fs_nodes_end};

  const char* strings_begin = TRY(rarcGetFSNames(node_info));
  if (!RangeContains(data, strings_begin))
    return std::unexpected("String table has invalid offset!");

  const char* strings_end =
      reinterpret_cast<const char*>(node_info) + result.header.files.offset;
  if (!RangeContainsInclusive(data, strings_end))
    return std::unexpected("Incomplete string table!");

  result.strings = {strings_begin, strings_end};

  auto* fd_begin = TRY(
      rarcGetFileData(reinterpret_cast<const rarcMetaHeader*>(data.data())));
  if (!RangeContains(data, fd_begin))
    return std::unexpected("Resource data has invalid offset");

  result.file_data = {fd_begin, data.data() + data.size()};

  return {};
}

using children_info_type =
    std::unordered_map<std::string, std::pair<std::size_t, std::size_t>>;

struct SpecialDirs {
  ResourceArchive::Node self;
  ResourceArchive::Node parent;
};

static Result<SpecialDirs>
rarcCreateSpecialDirs(const ResourceArchive::Node& node,
                      std::optional<ResourceArchive::Node> parent) {
  if (!node.is_folder())
    return std::unexpected("Can't make special dirs for a file!");

  SpecialDirs nodes;

  // This is the root
  if (!parent) {
    nodes.self = {
        .id = node.id,
        .flags = ResourceAttribute::DIRECTORY,
        .name = ".",
        .folder = {.parent = -1, .sibling_next = node.folder.sibling_next}};
    nodes.parent = {.id = -1,
                    .flags = ResourceAttribute::DIRECTORY,
                    .name = "..",
                    .folder = {.parent = -1, .sibling_next = 0}};
  } else {
    nodes.self = {.id = node.id,
                  .flags = ResourceAttribute::DIRECTORY,
                  .name = ".",
                  .folder = {.parent = parent->id,
                             .sibling_next = node.folder.sibling_next}};
    nodes.parent = {.id = parent->id,
                    .flags = ResourceAttribute::DIRECTORY,
                    .name = "..",
                    .folder = {.parent = parent->folder.parent,
                               .sibling_next = parent->folder.sibling_next}};
  }

  return nodes;
}

static Result<void>
rarcInsertSpecialDirs(std::vector<ResourceArchive::Node>& nodes) {
  std::vector<std::pair<int, int>> folder_infos;
  std::size_t adjust_amount = 2;

  for (auto node = nodes.begin(); node != nodes.end(); node++) {
    if (!node->is_folder() || rarcIsSpecialPath(node->name))
      continue;

    auto this_index = std::distance(nodes.begin(), node);

    for (auto& info : folder_infos) {
      auto& folder = nodes[info.first];

      // This folder is not a parent or future sibling of the new folder
      if (folder.folder.sibling_next <= this_index)
        continue;

      // Adjust for the subdir's special dirs
      info.second += 2;
    }

    folder_infos.push_back({this_index, adjust_amount});
    adjust_amount += 2;
  }

  for (auto info = folder_infos.rbegin(); info != folder_infos.rend(); info++) {
    auto& node = nodes[info->first];
    node.folder.sibling_next += info->second;

    SpecialDirs dirs;
    if (node.folder.parent != -1) {
      auto parent = std::find_if(
          nodes.begin(), nodes.end(), [&](const ResourceArchive::Node& n) {
            return n.is_folder() && !rarcIsSpecialPath(n.name) &&
                   n.id == node.folder.parent;
          });
      dirs = TRY(rarcCreateSpecialDirs(node, *parent));
    } else {
      dirs = TRY(rarcCreateSpecialDirs(node, std::nullopt));
    }

    nodes.insert(nodes.begin() + info->first + 1, dirs.parent);
    nodes.insert(nodes.begin() + info->first + 1, dirs.self);
  }

  return {};
}

static void rarcRemoveSpecialDirs(std::vector<ResourceArchive::Node>& nodes) {
  std::vector<std::pair<int, int>> folder_infos;

  for (auto node = nodes.begin(); node != nodes.end();) {
    if (!node->is_folder() || rarcIsSpecialPath(node->name))
      continue;

    for (auto info : folder_infos) {
      auto& folder = nodes[info.first];

      // This folder is not a parent or future sibling of the new folder
      if (folder.folder.sibling_next <= std::distance(nodes.begin(), node))
        continue;

      // Adjust for the subdir's special dirs
      info.second += 2;
    }

    folder_infos.push_back({std::distance(nodes.begin(), node), 2});
  }

  for (auto info = folder_infos.rbegin(); info != folder_infos.rend();) {
    auto& node = nodes[info->first];
    node.folder.sibling_next -= info->second;

    // Erase the special dirs
    nodes.erase(nodes.begin() + info->first + 1,
                nodes.begin() + info->first + 3);
  }
}

static void
rarcSortNodesForSaveRecursive(const std::vector<ResourceArchive::Node>& src,
                              std::vector<ResourceArchive::Node>& out,
                              children_info_type& children_info,
                              std::size_t node_index) {
  if (node_index > src.size() - 2) // Account for the children after
    return;

  const auto& node = src.at(node_index);

  assert(node.folder.sibling_next <= src.size()); // Account for end

  std::vector<ResourceArchive::Node> contents;
  std::vector<ResourceArchive::Node> dirs, sub_nodes, files, special_dirs;

  // Now push all children of this folder into their respective vectors
  auto begin = src.begin() + node_index + 1;
  auto end = src.begin() + node.folder.sibling_next;

  for (auto& child_node = begin; child_node != end; ++child_node) {
    if (child_node->is_folder()) {
      if (rarcIsSpecialPath(child_node->name)) {
        special_dirs.push_back(*child_node);
      } else {
        dirs.push_back(*child_node);
        std::vector<ResourceArchive::Node> nodes;
        rarcSortNodesForSaveRecursive(src, nodes, children_info,
                                      std::distance(src.begin(), child_node));
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

static Result<std::vector<ResourceArchive::Node>>
rarcProcessNodesForSave(const ResourceArchive& arc,
                        children_info_type& children_info) {
  std::vector<ResourceArchive::Node> out;

  assert(arc.nodes.size() > 0 && arc.nodes[0].folder.parent == -1);
  {
    std::vector<ResourceArchive::Node> with_dirs;
    with_dirs.insert(with_dirs.end(), arc.nodes.begin(), arc.nodes.end());
    TRY(rarcInsertSpecialDirs(with_dirs));
    out.push_back(arc.nodes[0]);
    rarcSortNodesForSaveRecursive(with_dirs, out, children_info, 0);
  }
  return out;
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

  for (auto* fs_node = fs_begin; fs_node != fs_end; ++fs_node) {
    if (rarcNodeIsFolder(*fs_node)) {
      std::string name = low.strings.data() + fs_node->name;
      if (rarcIsSpecialPath(name))
        continue;

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

  dir_node.folder.sibling_next = out.size() + depth + 1;
  out.insert(out.begin() + start_nodes, dir_node);
}

// EXTERNAL //

Result<bool> IsDataResourceArchive(rsl::byte_view data) {
  EXPECT(data.size() >= 4);
  return data[0] == 'R' && data[1] == 'A' && data[2] == 'R' && data[3] == 'C';
}

Result<ResourceArchive> LoadResourceArchive(rsl::byte_view data) {
  ResourceArchive result;
  LowResourceArchive low;

  TRY(LoadResourceArchiveLow(low, data));

  rarcRecurseLoadDirectory(result, low, low.dir_nodes[0], std::nullopt,
                           std::nullopt, result.nodes, 0);
  return result;
}

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
  auto processed_nodes = TRY(rarcProcessNodesForSave(arc, children_infos));

  std::string strings_blob;
  {
    offsets_map["."] = {0, 0};
    offsets_map[".."] = {2, 0};
    strings_blob += "." + std::string("\0", 1) + ".." + std::string("\0", 1);
    std::size_t fs_info_offset = 0;
    for (auto& node : arc.nodes) {
      if (make_matching) {
        offsets_map[node.name + std::to_string(node.id)] = {strings_blob.size(),
                                                            fs_info_offset};
        strings_blob += node.name + std::string("\0", 1);
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
    OffsetInfo offsets =
        offsets_map[make_matching ? node.name + std::to_string(node.id)
                                  : node.name];

    if (!node.is_folder())
      continue;

    rarcDirectoryNode low_dir;

    std::string s_magic = node.name;
    std::transform(s_magic.begin(), s_magic.end(), s_magic.begin(), ::toupper);
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

  // The FS node table however does not follow DFS :P

  // We store a vector of each used data offset to know if we need
  // to actually increment the data size marker. This is because
  // many files can point to the same piece of data to save space
  std::vector<u8> low_data;
  std::vector<std::size_t> used_offsets;
  for (auto& node : processed_nodes) {
    const bool is_special_dir = rarcIsSpecialPath(node.name);
    OffsetInfo offsets = offsets_map[make_matching && !is_special_dir
                                         ? node.name + std::to_string(node.id)
                                         : node.name];

    rarcFSNode low_node = {.hash = calc_key_code(node.name),
                           .type = node.flags << 8,
                           .name = offsets.string_offset};

    if (node.is_folder()) {
      // Skip the root.
      if (!is_special_dir && node.folder.parent == -1) {
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
          processed_nodes.begin(), processed_nodes.end(), [&](auto& n) {
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

Result<void> RecalculateArchiveIDs(ResourceArchive& arc) {
  std::vector<int> parent_stack = {
      -1,
  };
  std::vector<int> walk_stack = {};
  int folder_id = 0;

  for (auto node = arc.nodes.begin(); node != arc.nodes.end(); node++) {
    for (auto walk = walk_stack.begin(); walk != walk_stack.end();) {
      if ((*walk) == std::distance(arc.nodes.begin(), node)) {
        walk = walk_stack.erase(walk);
        parent_stack.pop_back();
      } else {
        walk++;
      }
    }
    if (node->is_folder()) {
      node->id = folder_id++;
      node->folder.parent = parent_stack.back();
      walk_stack.push_back(node->folder.sibling_next);
      parent_stack.push_back(node->id);
    }
  }

  children_info_type throwaway;
  const std::vector<ResourceArchive::Node>& calc_list =
      TRY(rarcProcessNodesForSave(arc, throwaway));

  s16 file_id = -1; // Adjust for root node
  for (auto& node : calc_list) {
    if (node.is_folder()) {
      file_id++;
      continue;
    }

    for (auto& target : arc.nodes) {
      if (target.id == node.id && target.name == node.name) {
        target.id = file_id++;
        break;
      }
    }
  }

  return {};
}

Result<void> ExtractResourceArchive(const ResourceArchive& arc,
                                    std::filesystem::path out) {
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

Result<ResourceArchive> CreateResourceArchive(std::filesystem::path root) {
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

  std::error_code err;

  paths.push_back(
      Entry{.str = (std::filesystem::path(".") / root.filename()).string(),
            .is_folder = true,
            .name = root.filename().string(),
            .parent = -1});

  std::vector<std::filesystem::path> sorted_fs_tree;
  GetSortedDirectoryListR(root, sorted_fs_tree);

  for (auto& path : sorted_fs_tree) {
    bool folder = std::filesystem::is_directory(path, err);
    if (err) {
      fmt::print(stderr, "CREATE: {}\n", err.message().c_str());
      continue;
    }
    if (path.filename() == ".DS_Store") {
      continue;
    }
    std::vector<u8> data;
    if (!folder) {
      auto f = ReadFile(path.string());
      if (!f) {
        return std::unexpected(
            std::format("CREATE: Failed to read file {}", path.string()));
      }
      data = *f;
    }

    path = std::filesystem::path(".") /
           std::filesystem::relative(path, root.parent_path());

    // We normalize it to lowercase because otherwise games can't find it.
    {
      auto path_string = path.string();
      std::transform(path_string.begin(), path_string.end(),
                     path_string.begin(),
                     [](char ch) { return std::tolower(ch); });
      path = std::filesystem::path(path_string);
    }

    paths.push_back(Entry{
        .str = path.string(),
        .is_folder = folder,
        .data = data,
        .name = path.filename().string(),
    });
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
    fmt::print(stdout, "CREATE: PATH={} (folder:{}, depth:{})\n", p.str,
               p.is_folder, p.depth);
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
      node.id = folder_id++;
      node.folder.parent =
          parent_stack.size() > 0 ? parent_stack[p.depth - 1].id : -1;
      node.folder.sibling_next = p.siblingNext;
      if (p.depth >= parent_stack.size()) {
        parent_stack.push_back(node);
      }
      node.flags |= ResourceAttribute::DIRECTORY;
      file_id += 2; // Account for hidden special dirs
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

  TRY(RecalculateArchiveIDs(result));
  return result;
}

Result<std::error_code> ImportFiles(ResourceArchive& rarc,
                                    ResourceArchive::Node parent,
                                    std::vector<rsl::File>& files) {
  if (files.size() == 0)
    return std::error_code();

  std::vector<ResourceArchive::Node> new_nodes;
  for (auto& file : files) {
    auto file_name = file.path.filename().string();
    std::transform(file_name.begin(), file_name.end(), file_name.begin(),
                   [](u8 ch) { return std::tolower(ch); });
    ResourceArchive::Node node = {.id = 0,
                                  .flags = ResourceAttribute::FILE |
                                           ResourceAttribute::PRELOAD_TO_MRAM,
                                  .name = file_name,
                                  .data = file.data};
    new_nodes.push_back(node);
  }

  int parent_index =
      std::distance(rarc.nodes.begin(),
                    std::find(rarc.nodes.begin(), rarc.nodes.end(), parent));

  // Manual sort is probably better because we can dodge DFS nonsense
  // effectively Even with O(n^2) because there typically aren't many nodes
  // anyway.
  for (auto new_node = new_nodes.begin(); new_node != new_nodes.end();
       new_node++) {
    auto dir_files_start = rarc.nodes.begin() + parent_index + 1;
    auto dir_files_end = rarc.nodes.begin() + parent.folder.sibling_next +
                         std::distance(new_nodes.begin(), new_node);
    for (auto child = dir_files_start; /*nothing*/; child++) {
      // Always insert at the very end (alphabetically last)
      if (child == dir_files_end) {
        rarc.nodes.insert(child, *new_node);
        break;
      }
      if (child->is_folder()) {
        // Always insert before the regular dirs
        rarc.nodes.insert(child, *new_node);
        break;
      }
      if (new_node->name < child->name) {
        rarc.nodes.insert(child, *new_node);
        break;
      }
    }
  }

  for (auto& node : rarc.nodes) {
    if (!node.is_folder())
      continue;
    if (node.folder.sibling_next <= parent_index)
      continue;
    node.folder.sibling_next += files.size();
  }

  return std::error_code();
}

Result<std::error_code> ImportFolder(ResourceArchive& rarc,
                                     ResourceArchive::Node parent,
                                     const std::filesystem::path& folder) {
  // Generate an archive so we can steal the DFS structure.
  auto tmp_rarc = TRY(librii::RARC::CreateResourceArchive(folder));

  int parent_index =
      std::distance(rarc.nodes.begin(),
                    std::find(rarc.nodes.begin(), rarc.nodes.end(), parent));

  auto insert_index = rarc.nodes.size();
  auto dir_files_start = rarc.nodes.begin() + parent_index + 1;
  auto dir_files_end = rarc.nodes.begin() + parent.folder.sibling_next;
  for (auto child = dir_files_start; /*nothing*/;) {
    if (child == dir_files_end) {
      // Always insert at the very end (alphabetically last)
      insert_index = std::distance(rarc.nodes.begin(), child);
      for (auto& new_node : tmp_rarc.nodes) {
        if (new_node.is_folder())
          new_node.folder.sibling_next += insert_index;
      }
      rarc.nodes.insert(child, tmp_rarc.nodes.begin(), tmp_rarc.nodes.end());
      break;
    }

    // Never insert in the files
    if (!child->is_folder()) {
      child++;
      continue;
    }

    // We are attempting to insert the entire
    // RARC fs as a new folder here
    if (tmp_rarc.nodes[0].name < child->name) {
      insert_index = std::distance(rarc.nodes.begin(), child);
      for (auto& new_node : tmp_rarc.nodes) {
        if (new_node.is_folder())
          new_node.folder.sibling_next += insert_index;
      }
      rarc.nodes.insert(child, tmp_rarc.nodes.begin(), tmp_rarc.nodes.end());
      break;
    }

    // Skip to the next dir (DFS means we jump subnodes)
    child = rarc.nodes.begin() + child->folder.sibling_next;
  }

  for (auto node = rarc.nodes.begin(); node != rarc.nodes.end();) {
    auto index = std::distance(rarc.nodes.begin(), node);
    // Skip files and the special dirs (they get recalculated later)
    if (!node->is_folder()) {
      node++;
      continue;
    }
    if (index == insert_index) { // Skip inserted dir (already adjusted)
      node = rarc.nodes.begin() + node->folder.sibling_next;
      continue;
    }
    if (node->folder.sibling_next <
        insert_index) { // Skip dirs that come before insertion by span alone
      node = rarc.nodes.begin() + node->folder.sibling_next;
      continue;
    }
    // This is a directory adjacent but not beyond the insertion, ignore.
    if (node->folder.parent >= parent.id && index < insert_index) {
      node = rarc.nodes.begin() + node->folder.sibling_next;
      continue;
    }
    node->folder.sibling_next += tmp_rarc.nodes.size();
    node++;
  }

  return std::error_code();
}

Result<void> CreateFolder(ResourceArchive& rarc, ResourceArchive::Node parent,
                          std::string name) {
  int parent_index =
      std::distance(rarc.nodes.begin(),
                    std::find(rarc.nodes.begin(), rarc.nodes.end(), parent));

  std::transform(name.begin(), name.end(), name.begin(),
                 [](char ch) { return std::tolower(ch); });

  ResourceArchive::Node folder_node = 
      {.id = 0,
       .flags = ResourceAttribute::DIRECTORY,
       .name = name,
       .folder = {.parent = parent.id, .sibling_next = 0}};

  auto insert_index = rarc.nodes.size();
  auto dir_files_start = rarc.nodes.begin() + parent_index + 1;
  auto dir_files_end = rarc.nodes.begin() + parent.folder.sibling_next;
  for (auto child = dir_files_start; /*nothing*/;) {
    if (child == dir_files_end) {
      // Always insert at the very end (alphabetically last)
      insert_index = std::distance(rarc.nodes.begin(), child);
      folder_node.folder.sibling_next = insert_index + 1;
      rarc.nodes.insert(child, folder_node);
      break;
    }

    // Never insert in the files
    if (!child->is_folder()) {
      child++;
      continue;
    }

    // We are attempting to insert the entire
    // RARC fs as a new folder here
    if (name < child->name) {
      insert_index = std::distance(rarc.nodes.begin(), child);
      folder_node.folder.sibling_next = insert_index + 1;
      rarc.nodes.insert(child, folder_node);
      break;
    }

    // Skip to the next dir (DFS means we jump subnodes)
    child = rarc.nodes.begin() + child->folder.sibling_next;
  }

  for (auto node = rarc.nodes.begin(); node != rarc.nodes.end();) {
    auto index = std::distance(rarc.nodes.begin(), node);
    // Skip files (they get recalculated later)
    if (!node->is_folder()) {
      node++;
      continue;
    }
    if (index == insert_index) { // Skip inserted dir (already adjusted)
      node = rarc.nodes.begin() + node->folder.sibling_next;
      continue;
    }
    if (node->folder.sibling_next <
        insert_index) { // Skip dirs that come before insertion by span alone
      node = rarc.nodes.begin() + node->folder.sibling_next;
      continue;
    }
    // This is a directory adjacent but not beyond the insertion, ignore.
    if (node->folder.parent >= parent.id && index < insert_index) {
      node = rarc.nodes.begin() + node->folder.sibling_next;
      continue;
    }
    node->folder.sibling_next += 1;
    node++;
  }

  return {};
}

bool DeleteNodes(ResourceArchive& rarc,
                 std::vector<ResourceArchive::Node>& nodes) {
  if (nodes.size() == 0)
    return false;

  for (auto& to_delete : nodes) {
    for (int i = 0; i < rarc.nodes.size(); i++) {
      if (to_delete == rarc.nodes[i]) {
        if (!to_delete.is_folder()) {
          ResourceArchive::Node parent;
          int parent_index = 0;
          for (parent_index = i; parent_index >= 0; parent_index--) {
            parent = rarc.nodes[parent_index];
            if (!parent.is_folder())
              continue;
            // This folder should be the deepest parent
            if (parent.folder.sibling_next > i)
              break;
          }
          auto deleted_at = std::distance(
              rarc.nodes.begin(), rarc.nodes.erase(rarc.nodes.begin() + i));

          for (auto& node : rarc.nodes) {
            if (!node.is_folder())
              continue;
            if (node.folder.sibling_next <= deleted_at)
              continue;
            node.folder.sibling_next--;
          }
        } else {
          auto begin = rarc.nodes.begin() + i;
          auto end = rarc.nodes.begin() + to_delete.folder.sibling_next;
          auto size = std::distance(begin, end);

          auto deleted_at =
              std::distance(rarc.nodes.begin(), rarc.nodes.erase(begin, end));

          for (auto& node : rarc.nodes) {
            if (!node.is_folder())
              continue;
            if (node.folder.sibling_next <= deleted_at)
              continue;
            if (node.folder.parent > to_delete.id)
              node.folder.parent -= 1;
            node.folder.sibling_next -= size;
          }
        }
      }
    }
  }

  return true;
}

Result<std::error_code> ExtractNodeTo(const ResourceArchive& rarc,
                                      ResourceArchive::Node node,
                                      const std::filesystem::path& dst) {
  std::error_code err;
  if (!std::filesystem::is_directory(dst, err))
    return std::unexpected("EXTRACT: Not a directory!");

  // Filesystem error
  if (err)
    return err;

  if (node.is_folder()) {
    ResourceArchive tmp_rarc{};

    auto node_it = std::find(rarc.nodes.begin(), rarc.nodes.end(), node);
    if (node_it == rarc.nodes.end())
      return std::unexpected("EXTRACT: Target node not found!");

    auto before_size = std::distance(rarc.nodes.begin(), node_it);
    auto parent_diff = node_it->id;

    tmp_rarc.nodes.insert(tmp_rarc.nodes.end(), node_it,
                          rarc.nodes.begin() + node.folder.sibling_next);

    tmp_rarc.nodes[0].folder = {-1, tmp_rarc.nodes[0].folder.sibling_next -
                                        (s32)before_size};

    tmp_rarc.nodes[1].folder = {-1, tmp_rarc.nodes[0].folder.sibling_next};

    tmp_rarc.nodes[2].folder = {-1, 0};
    for (auto tnode = tmp_rarc.nodes.begin() + 3; tnode != tmp_rarc.nodes.end();
         tnode++) {
      if (tnode->is_folder()) {
        tnode->folder.parent -= parent_diff;
        tnode->folder.sibling_next -= before_size;
      }
    }
    RecalculateArchiveIDs(tmp_rarc);

    TRY(librii::RARC::ExtractResourceArchive(tmp_rarc, dst));
    return std::error_code();
  }

  auto dst_path = dst / node.name;
  auto out = std::ofstream(dst_path.string(), std::ios::binary | std::ios::ate);

  out.write((const char*)node.data.data(), node.data.size());
  return std::error_code();
}

Result<std::error_code> ReplaceNode(ResourceArchive& rarc,
                                    ResourceArchive::Node to_replace,
                                    const std::filesystem::path& src) {
  std::error_code err;
  if (!std::filesystem::exists(src, err))
    return std::unexpected("REPLACE: Path doesn't exist!");

  // Filesystem error
  if (err)
    return err;

  auto node_it = std::find(rarc.nodes.begin(), rarc.nodes.end(), to_replace);
  if (node_it == rarc.nodes.end())
    return std::unexpected("Replace: Target node not found!");

  if (to_replace.is_folder()) {
    if (!std::filesystem::is_directory(src, err))
      return std::unexpected("REPLACE: Not a directory!");

    // Filesystem error
    if (err)
      return err;

    auto begin = node_it;
    auto end = rarc.nodes.begin() + to_replace.folder.sibling_next;
    auto old_size = std::distance(begin, end);

    auto deleted_at =
        std::distance(rarc.nodes.begin(), rarc.nodes.erase(begin, end));

    // Generate an archive so we can steal the DFS structure.
    auto tmp_rarc = TRY(librii::RARC::CreateResourceArchive(src));

    tmp_rarc.nodes[0].name = to_replace.name;
    tmp_rarc.nodes[0].folder.parent =
        to_replace.folder.parent; // This repairs the insertion node which will
                                  // recursively regenerate later

    for (auto& new_node : tmp_rarc.nodes) {
      if (new_node.is_folder())
        new_node.folder.sibling_next += deleted_at;
    }
    rarc.nodes.insert(rarc.nodes.begin() + deleted_at, tmp_rarc.nodes.begin(),
                      tmp_rarc.nodes.end());

    auto parent = std::find_if(rarc.nodes.begin(), rarc.nodes.end(),
                               [&](ResourceArchive::Node node) {
                                 return node.id == to_replace.folder.parent;
                               });

    auto sibling_adjust = tmp_rarc.nodes.size() - old_size;
    if (sibling_adjust != 0) {
      for (auto node = rarc.nodes.begin(); node != rarc.nodes.end();) {
        auto index = std::distance(rarc.nodes.begin(), node);
        // Skip files and the special dirs (they get recalculated later)
        if (!node->is_folder()) {
          node++;
          continue;
        }
        if (index == deleted_at) { // Skip inserted dir (already adjusted)
          node = rarc.nodes.begin() + node->folder.sibling_next;
          continue;
        }
        if (node->folder.sibling_next <
            deleted_at) { // Skip dirs that come before insertion by span
                          // alone
          node = rarc.nodes.begin() + node->folder.sibling_next;
          continue;
        }
        // This is a directory adjacent but not beyond the insertion, ignore.
        if (node->folder.parent >= parent->id && index < deleted_at) {
          node = rarc.nodes.begin() + node->folder.sibling_next;
          continue;
        }
        node->folder.sibling_next += sibling_adjust;
        node++;
      }
    }

    return std::error_code();
  }

  if (!std::filesystem::is_regular_file(src, err))
    return std::unexpected("REPLACE: Not a file!");

  // Filesystem error
  if (err)
    return err;

  auto in = std::ifstream(src.string(), std::ios::binary | std::ios::in);
  auto fsize = std::filesystem::file_size(src, err);

  // Filesystem error
  if (err)
    return err;

  node_it->data.resize(fsize);
  in.read((char*)node_it->data.data(), fsize);

  return std::error_code();
}

} // namespace librii::RARC
