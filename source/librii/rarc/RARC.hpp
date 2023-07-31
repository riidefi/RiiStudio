#pragma once

#include <array>
#include <core/common.h>
#include <filesystem>
#include <rsl/FsDialog.hpp>
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
  struct FolderInfo {
    s32 parent;
    s32 sibling_next;

    bool operator==(const FolderInfo& rhs) const = default;
  };

  struct Node {
    s32 id;
    u16 flags;
    std::string name;

    FolderInfo folder;
    std::vector<u8> data;

    bool is_folder() const { return (flags & DIRECTORY) != 0; }

    bool operator==(const Node& rhs) const = default;
  };

  std::vector<Node> nodes;
};

Result<std::pair<ResourceArchive::Node, ResourceArchive::Node>>
CreateSpecialDirs(const ResourceArchive::Node& node,
                  std::optional<ResourceArchive::Node> parent);

bool IsDataResourceArchive(rsl::byte_view data);

Result<ResourceArchive> LoadResourceArchive(rsl::byte_view data);
Result<std::vector<u8>> SaveResourceArchive(const ResourceArchive& arc,
                                            bool make_matching = true,
                                            bool ids_synced = true);

Result<void> RecalculateArchiveIDs(ResourceArchive& arc);

Result<void> ExtractResourceArchive(const ResourceArchive& arc,
                                    std::filesystem::path out);
Result<ResourceArchive> CreateResourceArchive(std::filesystem::path root);

Result<std::error_code> ImportFiles(ResourceArchive& rarc,
                                    ResourceArchive::Node parent,
                                    std::vector<rsl::File>& files);
Result<std::error_code> ImportFolder(ResourceArchive& rarc,
                                     ResourceArchive::Node parent,
                                     const std::filesystem::path& folder);
Result<void> CreateFolder(ResourceArchive& rarc, ResourceArchive::Node parent,
                          std::string name);
bool DeleteNodes(ResourceArchive& rarc,
                 std::vector<ResourceArchive::Node>& nodes);
Result<std::error_code> ExtractNodeTo(const ResourceArchive& rarc,
                                      ResourceArchive::Node node,
                                      const std::filesystem::path& dst);
Result<std::error_code> ReplaceNode(ResourceArchive& rarc,
                                    ResourceArchive::Node to_replace,
                                    const std::filesystem::path& src);

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
