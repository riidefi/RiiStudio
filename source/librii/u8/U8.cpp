#include "U8.hpp"
#include <core/common.h>
#include <core/util/oishii.hpp>
#include <core/util/timestamp.hpp>
#include <fstream>
#include <rsl/SimpleReader.hpp>

IMPORT_STD;

namespace librii::U8 {

struct rvlArchiveHeader {
  rsl::bu32 magic; // 00
  struct {
    rsl::bs32 offset; // 04
    rsl::bs32 size;   // 08
  } nodes;
  struct {
    rsl::bs32 offset; // 0C
  } files;

  std::array<u8, 16> watermark;
};

static_assert(sizeof(rvlArchiveHeader) == 32);

struct rvlArchiveNode {
  rsl::bu32 packed_type_name;
  union {
    struct {
      rsl::bu32 offset;
      rsl::bu32 size;
    } file;
    struct {
      rsl::bu32 parent;
      rsl::bu32 sibling_next;
    } folder;
  };
};

static_assert(sizeof(rvlArchiveNode) == 12);

struct LowU8Archive {
  rvlArchiveHeader header;
  std::vector<rvlArchiveNode> nodes;
  std::string strings;       // One giant string
  std::vector<u8> file_data; // One giant buffer
};

#define rvlArchiveNodeIsFolder(node) ((node).packed_type_name & 0xff000000)
#define rvlArchiveNodeGetName(node) ((node).packed_type_name & 0x00ffffff)

static inline Result<const rvlArchiveNode*>
rvlArchiveHeaderGetNodes(const rvlArchiveHeader* self) {
  EXPECT(self->nodes.offset >= sizeof(rvlArchiveHeader), "Invalid nodes");
  return (const rvlArchiveNode*)(((u8*)self) + self->nodes.offset);
}
static inline Result<const u8*>
rvlArchiveHeaderGetFileData(const rvlArchiveHeader* self) {
  EXPECT(self->files.offset >= sizeof(rvlArchiveHeader), "Invalid file data buffer");
  return (const u8*)((u8*)self + self->files.offset);
}

template <typename T>
static Result<void> SafeMemCopy(T& dest, rsl::byte_view data,
                                std::string fail_msg) {
  EXPECT(data.size_bytes() >= sizeof(T));
  std::memcpy(&dest, data.data(), sizeof(T));
}

static bool RangeContains(rsl::byte_view range, const void* ptr) {
  return ptr >= range.data() && ptr < (range.data() + range.size());
}
static bool RangeContainsInclusive(rsl::byte_view range, const void* ptr) {
  return ptr >= range.data() && ptr <= range.data() + range.size();
}

bool IsDataU8Archive(rsl::byte_view data) {
  return data[0] == 'U' && data[1] == u8('\xAA') && data[2] == '8' &&
         data[3] == '-';
}

Result<void> LoadU8Archive(LowU8Archive& result, rsl::byte_view data) {
  TRY(SafeMemCopy(result.header, data, "Invalid header"));

  const auto* nodes = TRY(rvlArchiveHeaderGetNodes(
      reinterpret_cast<const rvlArchiveHeader*>(data.data())));
  if (!RangeContains(data, nodes))
    return std::unexpected("Invalid nodes");

  const auto node_count = nodes[0].folder.sibling_next;
  if (!RangeContainsInclusive(data, nodes + node_count))
    return std::unexpected("Invalid root node");

  result.nodes = {nodes, nodes + node_count};

  const char* strings = reinterpret_cast<const char*>(nodes + node_count);
  if (!RangeContains(data, strings))
    return std::unexpected("Invalid strings");

  const char* strings_end =
      reinterpret_cast<const char*>(nodes) + result.header.nodes.size;
  if (!RangeContainsInclusive(data, strings_end))
    return std::unexpected("Invalid strings");

  result.strings = {strings, strings_end};

  auto* fd_begin = TRY(rvlArchiveHeaderGetFileData(
      reinterpret_cast<const rvlArchiveHeader*>(data.data())));
  if (!RangeContains(data, fd_begin))
    return std::unexpected("Invalid file data buffer");

  // For some reason the FD pointer is actually just the start of the file
  // fd_begin = data.data();
  int fd_trans = fd_begin - data.data();

  for (auto& node : result.nodes) {
    if (!rvlArchiveNodeIsFolder(node)) {
      node.file.offset = node.file.offset - fd_trans;
    }
  }

  result.file_data = {fd_begin, data.data() + data.size()};

  return {};
}

Result<U8Archive> LoadU8Archive(rsl::byte_view data) {
  U8Archive result;
  LowU8Archive low;
  TRY(LoadU8Archive(low, data));

  result.watermark = low.header.watermark;
  for (auto& node : low.nodes) {
    U8Archive::Node tmp = {.is_folder = (bool)rvlArchiveNodeIsFolder(node),
                           .name = low.strings.data() +
                                   rvlArchiveNodeGetName(node)};
    if (tmp.is_folder) {
      tmp.folder.parent = node.folder.parent;
      tmp.folder.sibling_next = node.folder.sibling_next;
    } else {
      tmp.file.offset = node.file.offset;
      tmp.file.size = node.file.size;
    }

    result.nodes.push_back(tmp);
  }

  result.file_data = std::move(low.file_data);
  return result;
}

std::vector<u8> SaveU8Archive(const U8Archive& arc) {
  std::string strings;
  std::unordered_map<std::string, std::size_t> strings_map;

  for (auto& node : arc.nodes) {
    strings_map[node.name] = strings.size();
    strings += node.name + std::string("\0", 1);
  }

  rvlArchiveHeader header{.magic = 0x55aa382d, .watermark = arc.watermark};

  header.nodes.offset = sizeof(rvlArchiveHeader);
  auto string_ofs =
      header.nodes.offset + sizeof(rvlArchiveNode) * arc.nodes.size();
  header.nodes.size = string_ofs + strings.size();
  header.files.offset = header.nodes.offset + header.nodes.size;

  std::vector<rvlArchiveNode> nodes;
  for (auto& node : arc.nodes) {
    std::array<u8, sizeof(rvlArchiveHeader)> _null{};
    rvlArchiveNode an = reinterpret_cast<rvlArchiveNode&>(_null);

    an.packed_type_name =
        (node.is_folder << 24) | (strings_map[node.name] & 0x00FF'FFFF);

    if (node.is_folder) {
      an.folder.parent = node.folder.parent;
      an.folder.sibling_next = node.folder.sibling_next;
    } else {
      an.file.offset = header.files.offset + node.file.offset;
      an.file.size = node.file.size;
    }

    nodes.push_back(an);
  }

  int total_file_size = 0;
  for (auto& node : arc.nodes) {
    if (!node.is_folder)
      total_file_size += node.file.size;
  }

  assert(total_file_size == arc.file_data.size());

  std::vector<u8> result(header.files.offset + total_file_size);

  memcpy(&result[0], &header, sizeof(header));
  memcpy(&result[header.nodes.offset], nodes.data(),
         nodes.size() * sizeof(rvlArchiveNode));
  memcpy(&result[string_ofs], strings.data(), strings.size());
  memcpy(&result[header.files.offset], arc.file_data.data(),
         arc.file_data.size());

  assert(result.size() == header.files.offset + total_file_size);
  return result;
}
inline bool __rxPathCompare(const char* lhs, const char* rhs) {
  while (rhs[0] != '\0') {
    if (tolower(*lhs++) != tolower(*rhs++))
      return false;
  }

  if (lhs[0] == '/' || lhs[0] == '\0')
    return true;

  return false;
}
s32 PathToEntrynum(const U8Archive& arc, const char* path, u32 currentPath) {
  s32 name_length;      // r7
  u32 it = currentPath; // r8

  while (true) {
    // End of string -> return what we have
    if (path[0] == '\0')
      return it;

    // Ignore initial slash: /Path/File vs Path/File
    if (path[0] == '/') {
      it = 0;
      ++path;
      continue;
    }

    // Handle special cases:
    // -../-, -.., -./-, -.
    if (path[0] == '.') {
      if (path[1] == '.') {
        // Seek to parent ../
        if (path[2] == '/') {
          it = arc.nodes[it].folder.parent;
          path += 3;
          continue;
        }
        // Return parent folder immediately
        if (path[2] == '\0')
          return arc.nodes[it].folder.parent;
        // Malformed: fall through, causing infinite loop
        goto compare;
      }

      // "." directory does nothing
      if (path[1] == '/') {
        path += 2;
        continue;
      }

      // Ignore trailing dot
      if (path[1] == '\0')
        return it;
    }

  compare:
    // We've ensured the directory is not special.
    // Isolate the name of the current item in the path string.
    const char* name_end = path;
    while (name_end[0] != '\0' && name_end[0] != '/')
      ++name_end;

    // If the name was delimited by a '/' rather than truncated.
    int name_delimited_by_slash = name_end[0] != '\0';
    name_length = name_end - path;

    // Traverse all children of the parent.
    const u32 anchor = it;
    ++it;
    while (it < arc.nodes[anchor].folder.sibling_next) {
      while (true) {
        if (arc.nodes[it].is_folder || !name_delimited_by_slash) {
          auto name = arc.nodes[it].name;
          // Skip empty directories
          if (name == ".") {
            ++it;
            continue;
          }

          // Advance to the next item in the path
          if (__rxPathCompare(path, name.c_str())) {
            goto descend;
          }
        }

        if (arc.nodes[it].is_folder) {
          it = arc.nodes[it].folder.sibling_next;
          break;
        }

        ++it;
        break;
      }
    }

    return -1;

  descend:
    // If the path was truncated, there is nowhere else to go
    // These basic blocks have to go here right at the end, accessed via a goto.
    // An odd choice.
    if (!name_delimited_by_slash)
      return it;

    path += name_length + 1;
  }
}

Result<void> Extract(const U8Archive& arc, std::filesystem::path out) {
  auto tmp = out;
  std::vector<u32> stack;
  u32 i = 0;
  for (auto& node : arc.nodes) {
    if (!stack.empty() && stack.back() == i) {
      stack.resize(stack.size() - 1);
      tmp = tmp.parent_path();
    }
    ++i;
    if (node.is_folder) {
      stack.push_back(node.folder.sibling_next);
      tmp /= node.name;
      std::filesystem::create_directory(tmp);
    } else {
      auto fpath = (tmp / node.name).string();
      std::ofstream out(fpath, std::ios::binary | std::ios::ate);
      auto data = std::span(arc.file_data.begin(), arc.file_data.end())
                      .subspan(node.file.offset, node.file.size);
      EXPECT(data.size() == node.file.size);
      out.write(reinterpret_cast<const char*>(data.data()), data.size());
    }
  }
  return {};
}

// Insertion into a sorted list would be O(n^2) on a vector structure; a heap or
// tree structure is probably not meritful. Instead, simply use std::sort
// (O(nlgn)) to get files in the proper order and then iterate through them.
Result<U8Archive> Create(std::filesystem::path root) {
  struct Entry {
    std::string str;
    bool is_folder = false;

    std::vector<u8> data;
    std::string name;

    int depth = -1;
    int parent = 0; // Default files put parent as 0 for root
    int nextAtGreaterDepth = -1;

    bool operator<(const Entry& rhs) const {
      return str < rhs.str && (is_folder ? 1 : 0) < (rhs.is_folder ? 1 : 0);
    }
  };
  std::vector<Entry> paths{{
      .str = ".",
      .is_folder = true,
  }};
  for (auto&& it : std::filesystem::recursive_directory_iterator{root}) {
    auto path = it.path();
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
    path = std::filesystem::path(".") / std::filesystem::relative(path, root);

    paths.push_back(Entry{
        .str = path.string(),
        .is_folder = folder,
        .data = data,
        .name = path.filename().string(),
    });
  }
  // They are almost certainly already in sorted order
  std::sort(paths.begin(), paths.end());
  for (auto& p : paths) {
    for (auto c : p.str) {
      // relative paths are weakly_canonical
      if (c == '/' || c == '\\') {
        ++p.depth;
      }
    }
  }
  for (size_t i = 0; i < paths.size(); ++i) {
    auto& p = paths[i];
    if (p.is_folder) {
      for (int j = i; j < paths.size(); ++j) {
        if (paths[j].depth <= p.depth) {
          p.nextAtGreaterDepth = j;
          break;
        }
        paths[j].parent = i;
      }
    }
  }
  for (auto& p : paths) {
    fmt::print("PATH: {} (folder:{}, depth:{})\n", p.str, p.is_folder, p.depth);
  }
  U8Archive result;
  std::array<char, 16> watermark{};
  std::format_to_n(watermark.data(), std::size(watermark), "{}",
                   std::string(VERSION_SHORT));
  static_assert(result.watermark.size() == watermark.size());
  memcpy(result.watermark.data(), watermark.data(), result.watermark.size());

  u32 size = 0;
  for (auto& p : paths) {
    size += p.data.size();
  }
  result.file_data.resize(size);

  u32 i = 0;
  for (auto& p : paths) {
    U8Archive::Node node;
    node.is_folder = p.is_folder;
    node.name = p.name;
    if (p.is_folder) {
      node.folder.parent = p.parent;
      node.folder.sibling_next = p.nextAtGreaterDepth;
    } else {
      node.file.offset = result.file_data.size();
      node.file.size = p.data.size();
      memcpy(result.file_data.data() + i, p.data.data(), p.data.size());
      i += p.data.size();
    }
    result.nodes.push_back(node);
  }

  return result;
}

} // namespace librii::U8
