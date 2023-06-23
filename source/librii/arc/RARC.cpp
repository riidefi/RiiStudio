#include "RARC.hpp"
#include <core/common.h>
#include <rsl/SimpleReader.hpp>
#include <iostream>

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
    rsl::bu32 data_size;
    } fs_nodes;
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
    rarcFSNode() : id(0xFFFF), hash(0), type(0), name(0), folder(), padding() {}

    rsl::bu16 id;
    rsl::bu16 hash;
    rsl::bu16 type;
    rsl::bu16 name;
    union {
    struct {
      rsl::bu32 dir_node;
      rsl::bu32 size;  // Always 0x10
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

static inline const rarcFSNode*
rarcGetFSNodes(const rarcNodesHeader* self) {
    assert(self->fs_nodes.offset >=
           self->dir_nodes.offset +
               (((sizeof(rarcDirectoryNode) * self->dir_nodes.count) + 0x1F) &
                ~0x1F));
    return (const rarcFSNode*)(((u8*)self) + self->fs_nodes.offset);
}

static inline const char* rarcGetFSNames(const rarcNodesHeader* self) {
    assert(
        self->strings_offset >=
        self->fs_nodes.offset +
            (((sizeof(rarcFSNode) * self->fs_nodes.count) + 0x1F) & ~0x1F));
    return ((const char*)self) + self->strings_offset;
}

static inline const u8*
rarcGetFileData(const rarcMetaHeader* self) {
    assert(self->files.offset >= sizeof(rarcMetaHeader));
    return (const u8*)((u8*)self + self->files.offset);
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

bool RangeContainsInclusive(rsl::byte_view range,
                            const void* ptr) {
  return ptr >= range.data() && ptr <= range.data() + range.size();
}

Result<LowResourceArchive> LoadResourceArchiveLow(rsl::byte_view data) {
  LowResourceArchive result;

  if (!SafeMemCopy(result.header, data))
    return std::unexpected("Incomplete meta header data!");

  const auto* node_info = rarcGetNodesHeader(
      reinterpret_cast<const rarcMetaHeader*>(data.data()));
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

  auto* fd_begin = rarcGetFileData(
      reinterpret_cast<const rarcMetaHeader*>(data.data()));
  if (!RangeContains(data, fd_begin))
    return std::unexpected("Resource data has invalid offset");

  result.file_data = {fd_begin, data.data() + data.size()};

  return result;
}

static void rarcRecurseLoadDirectory(ResourceArchive& arc, LowResourceArchive& low, rarcFSNode &node, std::size_t parent) {
  rarcDirectoryNode& d_n = low.dir_nodes.at(node.folder.dir_node - 2);
  ResourceArchive::Node tmp_d = {.flags = node.type >> 8,
                                 .name = low.strings.data() + d_n.name};

  std::vector<ResourceArchive::Node> tmp_files;

  auto* fs_begin = low.fs_nodes.data() + d_n.children_offset;
  auto* fs_end = low.fs_nodes.data() + d_n.children_offset + d_n.child_count;
  for (auto* fs_node = fs_begin; fs_node != fs_end; ++fs_node) {
    if (rarcNodeIsFolder(*fs_node)) {
      rarcRecurseLoadDirectory(arc, low, *fs_node, d_n.children_offset + (fs_node - fs_begin));
    } else {
      ResourceArchive::Node tmp_f = {.flags = fs_node->type >> 8,
                                     .name = low.strings.data() + fs_node->name};
      tmp_f.file.offset = fs_node->file.offset;
      tmp_f.file.size = fs_node->file.size;
      tmp_files.push_back(tmp_f);
	}
  }

  tmp_d.folder.sibling_next = fs_end->id;
  tmp_d.folder.parent = parent;

  arc.nodes.push_back(tmp_d);
  arc.nodes.insert(arc.nodes.end(), tmp_files.begin(), tmp_files.end());
}

Result<ResourceArchive> LoadResourceArchive(rsl::byte_view data) {
  ResourceArchive result;

  auto low_result = LoadResourceArchiveLow(data);
  if (!low_result)
    return std::unexpected(low_result.error());

  rarcRecurseLoadDirectory(result, *low_result, low_result->fs_nodes[0], -1);

  result.file_data = std::move(low_result->file_data);
  return result;
}

Result<std::vector<u8>> SaveResourceArchive(const ResourceArchive& arc) {
  std::string strings;

  struct OffsetInfo {
    std::size_t string_offset;
    std::size_t fs_node_offset;
  };
  std::unordered_map<std::string, OffsetInfo> offsets_map;

  {
    std::size_t fs_info_offset = 0;
    for (auto& node : arc.nodes) {
      offsets_map[node.name] = {strings.size(), fs_info_offset};
      strings += node.name + std::string("\0", 1);
      fs_info_offset += sizeof(rarcFSNode);
    }
  }

  std::vector<rarcDirectoryNode> dir_nodes;
  std::vector<rarcFSNode> fs_nodes;

  std::size_t dirs = 0, files = 0;
  std::size_t mram_size = 0, aram_size = 0, dvd_size = 0;
  for (auto &node : arc.nodes) {
    OffsetInfo offsets = offsets_map[node.name];

    rarcFSNode fn;
    fn.hash = calc_key_code(node.name);
    fn.name = offsets.string_offset;
    fn.type = node.flags << 8;

    if (node.is_folder()) {
      {
        rarcDirectoryNode dn;

        std::string s_magic = node.name;
        std::transform(s_magic.begin(), s_magic.end(), s_magic.begin(),
                       ::toupper);
        if (s_magic.size() < 4)
          s_magic.append(' ', 4 - s_magic.size());

        dn.magic = *reinterpret_cast<u32*>(s_magic.data());
        dn.name = offsets.string_offset;
        dn.child_count = node.folder.sibling_next - fs_nodes.size(); // Get child count (will require collecting tree structure
        dn.children_offset = offsets.fs_node_offset;

        dir_nodes.push_back(dn);
      }

	  fn.id = 0xFFFF;
	  fn.folder.dir_node = dirs;
      fn.folder.size = 0x10;

	  dirs += 1;
    } else {
      fn.id = files;
      fn.file.offset = node.file.offset;
      fn.file.size = node.file.size;
      if ((node.flags & ResourceAttribute::PRELOAD_TO_MRAM)) {
        mram_size += node.file.size;
      } else if ((node.flags & ResourceAttribute::PRELOAD_TO_ARAM)) {
        aram_size += node.file.size;
      } else if ((node.flags & ResourceAttribute::LOAD_FROM_DVD)) {
        dvd_size += node.file.size;
      } else {
        return std::unexpected(std::format("File \"{}\" hasn't been marked for loading!", node.name));
	  }
      files += 1;
    }

	fs_nodes.push_back(fn);
  }

  #if DEBUG
  for (auto& fs_node : fs_nodes) {
        std::cout << std::format("== FS NODE ==\n"
                                 "Name: {0}\n"
                                 "Type: {1}\n",
                                 (strings.data() + *fs_node.name), *fs_node.type);
  }
  #endif

  std::size_t total_file_size = mram_size + aram_size + dvd_size;
  if (total_file_size != arc.file_data.size())
    return std::unexpected("Marked total file size doesn't match true data size!");

  // Calculate dirs, file, and their names
  // TODO: Allow custom unsynced IDs?
  rarcNodesHeader node_header{.id_max = files, .ids_synced = true};
  node_header.dir_nodes.count = dirs;
  node_header.dir_nodes.offset = sizeof(rarcNodesHeader);
  node_header.fs_nodes.count = fs_nodes.size();
  node_header.fs_nodes.offset =
      (node_header.dir_nodes.count * sizeof(rarcDirectoryNode) + 0x1F) & ~0x1F;
  node_header.fs_nodes.data_size = 0;  // TODO: Properly calculate!
  node_header.strings_offset = (node_header.fs_nodes.offset +
                               (node_header.fs_nodes.count * sizeof(rarcFSNode)) + 0x1F) & ~0x1F;

  // Calculate header connections, data info, and size information
  rarcMetaHeader meta_header{.magic = 0x52415243};
  meta_header.nodes.offset = sizeof(rarcMetaHeader);
  meta_header.files.offset =
      (meta_header.nodes.offset + node_header.strings_offset + strings.size() + 0x1F) & ~0x1F;
  meta_header.files.size = total_file_size;
  meta_header.files.mram_size = mram_size;
  meta_header.files.aram_size = aram_size;
  meta_header.files.dvd_size = dvd_size;
  meta_header.size = meta_header.nodes.offset + meta_header.files.offset +
                     meta_header.files.size;

  std::vector<u8> result(meta_header.size);

  memcpy(&result[0], &meta_header, sizeof(rarcMetaHeader));
  memcpy(&result[meta_header.nodes.offset], &node_header, sizeof(rarcNodesHeader));
  memcpy(&result[meta_header.nodes.offset + node_header.dir_nodes.offset],
         dir_nodes.data(),
         sizeof(rarcDirectoryNode) * node_header.dir_nodes.count);
  memcpy(&result[meta_header.nodes.offset + node_header.strings_offset],
         strings.data(), strings.size());
  memcpy(&result[meta_header.nodes.offset + meta_header.files.offset],
         arc.file_data.data(), arc.file_data.size());

  if (result.size() != meta_header.size)
    return std::unexpected(
        "Saved data mismatched the recorded data size provided!");

  return result;
}

Result<void> Extract(const ResourceArchive& arc, std::filesystem::path out) {
  return Result<void>();
}

Result<ResourceArchive> Create(std::filesystem::path root) {
  return Result<ResourceArchive>();
}

} // namespace librii::RARC
