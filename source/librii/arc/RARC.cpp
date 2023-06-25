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
    auto* node_info = rarcGetNodesHeader(self);
	assert(self->files.offset >= node_info->strings_offset + node_info->string_table_size);
	return (const u8*)((((u8*)self) + ((self->nodes.offset + self->files.offset + 0x1F) & ~0x1F)));
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

Result<void>
LoadResourceArchiveLow(LowResourceArchive &result, rsl::byte_view data) {
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

	return {};
}

static std::vector<ResourceArchive::Node>
rarcRecurseLoadDirectory(ResourceArchive& arc,
						 LowResourceArchive& low,
						 rarcDirectoryNode& low_dir,
						 std::optional<rarcFSNode> low_node,
						 std::optional<ResourceArchive::Node> dir_parent) {
    ResourceArchive::Node dir_node;
	if (low_node) {
        dir_node = {.id = static_cast<s32>(low_node->folder.dir_node),
					.flags = static_cast<u16>(low_node->type >> 8),
                    .name = low.strings.data() + low_dir.name};
    } else {
		// This is the ROOT directory (exists without an fs node to further describe it)
        dir_node = {.id = 0,
					.flags = ResourceAttribute::DIRECTORY,
                    .name = low.strings.data() + low_dir.name};
	}
    dir_node.folder.parent = dir_parent ? dir_parent->id : -1;

    auto* fs_begin = low.fs_nodes.data() + (low_dir.children_offset);
    auto* fs_end =
        low.fs_nodes.data() + (low_dir.children_offset + low_dir.child_count);

	dir_node.folder.sibling_next = fs_end->id;

	ResourceArchive::Node dir_ref_node;
	ResourceArchive::Node dir_parent_ref_node;

    std::vector<ResourceArchive::Node> file_nodes;
    for (auto* fs_node = fs_begin; fs_node != fs_end; ++fs_node) {
        if (rarcNodeIsFolder(*fs_node)) {
            if ((low_node && fs_node->folder.dir_node == low_node->folder.dir_node) ||
                (!low_node && fs_node->folder.dir_node == 0)) {
				// Path "."
				dir_ref_node = {.id = fs_node->folder.dir_node,
								.flags =
									static_cast<u16>(fs_node->type >> 8),
								.name = low.strings.data() + fs_node->name};
                continue;
			}

            if ((dir_parent && fs_node->folder.dir_node == dir_parent->id) ||
                fs_node->folder.dir_node == -1) {
                // Path ".."
                dir_parent_ref_node = {
                    .id = dir_parent ? dir_parent->id : -1,
                    .flags = static_cast<u16>(fs_node->type >> 8),
                    .name = low.strings.data() + fs_node->name};
                continue;
            }

			// Sub Directory
			auto& chlld_dir_n = low.dir_nodes.at(fs_node->folder.dir_node);
			auto subnodes = rarcRecurseLoadDirectory(arc, low, chlld_dir_n, *fs_node, dir_node);
            file_nodes.insert(file_nodes.end(), subnodes.begin(), subnodes.end());
		} else {
			ResourceArchive::Node tmp_f = {.id = fs_node->id,
										   .flags = static_cast<u16>(fs_node->type >> 8),
                                           .name = low.strings.data() + fs_node->name};
			tmp_f.file.offset = fs_node->file.offset;
			tmp_f.file.size = fs_node->file.size;
			file_nodes.push_back(tmp_f);
		}
	}

	dir_ref_node.folder.parent = dir_node.folder.parent;
    dir_ref_node.folder.sibling_next = dir_node.folder.sibling_next;
    file_nodes.insert(file_nodes.end(), dir_ref_node);

    dir_parent_ref_node.folder.parent = dir_parent ? dir_parent->folder.parent : -1;
    dir_parent_ref_node.folder.sibling_next = dir_parent ? dir_parent->folder.sibling_next : 0;
    file_nodes.insert(file_nodes.end(), dir_parent_ref_node);

	file_nodes.insert(file_nodes.begin(), dir_node);
	return file_nodes;
}

Result<ResourceArchive> LoadResourceArchive(rsl::byte_view data) {
	ResourceArchive result;
    LowResourceArchive low;

	auto low_result = LoadResourceArchiveLow(low, data);
	if (!low_result)
	return std::unexpected(low_result.error());

    result.nodes = rarcRecurseLoadDirectory(result, low, low.dir_nodes[0],
                                            std::nullopt, std::nullopt);
    result.file_data = std::move(low.file_data);
	return result;
}

Result<std::vector<u8>> SaveResourceArchive(const ResourceArchive& arc) {
	struct OffsetInfo {
		std::size_t string_offset;
		std::size_t fs_node_offset;
	};
	std::unordered_map<std::string, OffsetInfo> offsets_map;

	std::string strings_blob;
	{
        offsets_map["."] = {0, 0};
        offsets_map[".."] = {2, 0};
        strings_blob += ".\0..\0";
		std::size_t fs_info_offset = 0;
		for (auto& node : arc.nodes) {
            if (!offsets_map.contains(node.name)) {
                offsets_map[node.name] = {strings_blob.size(), fs_info_offset};
                strings_blob += node.name + "\0";
            }
			fs_info_offset += sizeof(rarcFSNode);
		}
	}

	std::vector<rarcDirectoryNode> dir_nodes;
	std::vector<rarcFSNode> fs_nodes;

	// TODO: Organize the fs_nodes so that it is ordered as follows:
	// =============================================================
	// Folder >
	//   All Folders ...
	//   All Files ...
	//   Contents of Folders ... (Follows structure listed)
	//   Special Dirs (., ..)
	// =============================================================

	std::size_t mram_size = 0, aram_size = 0, dvd_size = 0;
	for (auto &node : arc.nodes) {
        std::cout << node.name << "\n";
		OffsetInfo offsets = offsets_map[node.name];

		rarcFSNode low_node = {.hash = calc_key_code(node.name),
                               .type = node.flags << 8,
                               .name = offsets.string_offset};

        if (node.is_folder()) {
			low_node.id = 0xFFFF;
			low_node.folder.dir_node = node.id;
			low_node.folder.size = 0x10;

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
				low_dir.child_count = node.folder.sibling_next - fs_nodes.size(); // Get child count (will require collecting tree structure
				low_dir.children_offset = fs_nodes.size();

				if (node.folder.parent != -1) {
                    low_dir.magic = *reinterpret_cast<rsl::bu32*>(s_magic.data());
                    fs_nodes.push_back(low_node);  // This pushes the directory into the fs list if it's not the root
                } else {
					low_dir.magic = static_cast<rsl::bu32>('ROOT');
				}

				dir_nodes.push_back(low_dir);
				continue;
			}

            fs_nodes.push_back(low_node);
		} else {
			// IDs are calculated by incrementing for each file and special dir
			// The difference of FS nodes and pure Dir nodes is this total
			low_node.id = node.id; // fs_nodes.size() - dir_nodes.size();
			low_node.file.offset = node.file.offset;
			low_node.file.size = node.file.size;
			if ((node.flags & ResourceAttribute::PRELOAD_TO_MRAM)) {
			mram_size += (node.file.size + 0x1F) & ~0x1F;
			} else if ((node.flags & ResourceAttribute::PRELOAD_TO_ARAM)) {
			aram_size += (node.file.size + 0x1F) & ~0x1F;
			} else if ((node.flags & ResourceAttribute::LOAD_FROM_DVD)) {
			dvd_size += (node.file.size + 0x1F) & ~0x1F;
			} else {
			return std::unexpected(std::format("File \"{}\" hasn't been marked for loading!", node.name));
			}
            fs_nodes.push_back(low_node);
		}
	}

	std::size_t total_file_size = mram_size + aram_size + dvd_size;
	if (total_file_size != arc.file_data.size())
	return std::unexpected("Marked total file size doesn't match true data size!");

	// Calculate dirs, file, and their names
	// TODO: Allow custom unsynced IDs?
	// 
	// Subtract 1 for the root directory (doesn't have a fs node)
	rarcNodesHeader node_header{.id_max = fs_nodes.size(), .ids_synced = true};
	node_header.dir_nodes.count = dir_nodes.size();
	node_header.dir_nodes.offset = sizeof(rarcNodesHeader);
	node_header.fs_nodes.count = fs_nodes.size();
	node_header.fs_nodes.offset = node_header.dir_nodes.offset +
		(node_header.dir_nodes.count * sizeof(rarcDirectoryNode) + 0x1F) & ~0x1F;
	node_header.string_table_size = (strings_blob.size() + 0x1F) & ~0x1F ;  // TODO: Properly calculate!
	node_header.strings_offset = (node_header.fs_nodes.offset +
								(node_header.fs_nodes.count * sizeof(rarcFSNode)) + 0x1F) & ~0x1F;

	// Calculate header connections, data info, and size information
    rarcMetaHeader meta_header{.magic = static_cast<rsl::bu32>('RARC')}; // Little endian 'RARC'
	meta_header.nodes.offset = sizeof(rarcMetaHeader);
	meta_header.files.offset =
		(meta_header.nodes.offset + node_header.strings_offset + strings_blob.size() + 0x1F) & ~0x1F;
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
    memcpy(&result[meta_header.nodes.offset + node_header.fs_nodes.offset],
           fs_nodes.data(),
           sizeof(rarcFSNode) * node_header.fs_nodes.count);
	memcpy(&result[meta_header.nodes.offset + node_header.strings_offset],
		   strings_blob.data(), strings_blob.size());
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
