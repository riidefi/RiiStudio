#pragma once

#include <array>
#include <core/common.h>
#include <rsl/SimpleReader.hpp>
#include <span>
#include <string>
#include <vector>

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

    struct {
      s32 parent;
      s32 sibling_next;
    } folder;

    std::vector<u8> data;

    bool is_folder() const { return (flags & DIRECTORY) != 0; }
    bool is_special_path() const { return name == "." || name == ".."; }

    bool operator==(const Node& rhs) const {
      return id == rhs.id && flags == rhs.flags && name == rhs.name &&
             folder.parent == rhs.folder.parent &&
             folder.sibling_next == rhs.folder.sibling_next && data == rhs.data;
    }
  };

  std::vector<Node> nodes;
};

bool IsDataResourceArchive(rsl::byte_view data);

Result<ResourceArchive> LoadResourceArchive(rsl::byte_view data);
Result<std::vector<u8>> SaveResourceArchive(const ResourceArchive& arc,
                                            bool make_matching = true,
                                            bool ids_synced = true);

void RecalculateArchiveIDs(ResourceArchive& arc);

Result<void> Extract(const ResourceArchive& arc, std::filesystem::path out);
Result<ResourceArchive> Create(std::filesystem::path root);

} // namespace librii::RARC
