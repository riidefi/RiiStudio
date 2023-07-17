#pragma once

#include <array>
#include <core/common.h>
#include <filesystem>
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

Result<void> ExtractResourceArchive(const ResourceArchive& arc,
                                    std::filesystem::path out);
Result<ResourceArchive> CreateResourceArchive(std::filesystem::path root);

struct ResourceArchiveNodeHasher {
  std::size_t operator()(const ResourceArchive::Node& node) const {
    std::size_t h1 = std::hash<s32>{}(node.id);
    std::size_t h2 = std::hash<std::string>{}(node.name);
    std::size_t h3 = 0;
    if (node.is_folder()) {
      h3 = std::hash<s32>{}(node.folder.parent);
    }
    // Combine hashes - This is a common technique to combine hash values
    return h1 ^ (h2 << 1) ^ (h3 << 2);
  }
};

} // namespace librii::RARC
