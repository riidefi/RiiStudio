#pragma once

#include <array>
#include <core/common.h>
#include <span>
#include <string>
#include <vector>
#include <rsl/SimpleReader.hpp>

namespace librii::RARC {

enum ResourceAttribute : u8 {
  FILE = 1 << 0,
  DIRECTORY = 1 << 1,
  COMPRESSED = 1 << 2,
  PRELOAD_TO_MRAM = 1 << 4,
  PRELOAD_TO_ARAM = 1 << 5,
  LOAD_FROM_DVD = 1 << 6,
  YAZ0_COMPRESSED = 1 << 7
};

struct ResourceArchive {
  struct Node {
    s32 id;
    u16 flags;
    std::string name;

    union {
      struct {
        s32 parent;
        s32 sibling_next;
      } folder;
      struct {
        u32 offset;
        u32 size;
      } file;
    };

	bool is_folder() const { return (flags & DIRECTORY) != 0; }
    bool is_special_path() const { return name == "." || name == ".."; }

	bool operator==(const Node& other) const {
      return id == other.id && name == other.name;
	}
  };

  std::vector<Node> nodes;
  std::vector<u8> file_data;
};

Result<ResourceArchive> LoadResourceArchive(rsl::byte_view data);
Result<std::vector<u8>> SaveResourceArchive(const ResourceArchive& arc,
                                            bool make_matching = true);

std::size_t OptimizeArchiveData(ResourceArchive& arc);

Result<void> Extract(const ResourceArchive& arc, std::filesystem::path out);
Result<ResourceArchive> Create(std::filesystem::path root);

} // namespace librii::RARC
