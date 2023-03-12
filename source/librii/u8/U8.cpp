#include "U8.hpp"
#include <core/common.h>
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

static inline const rvlArchiveNode*
rvlArchiveHeaderGetNodes(const rvlArchiveHeader* self) {
  assert(self->nodes.offset >= sizeof(rvlArchiveHeader));
  return (const rvlArchiveNode*)(((u8*)self) + self->nodes.offset);
}
static inline const u8*
rvlArchiveHeaderGetFileData(const rvlArchiveHeader* self) {
  assert(self->files.offset >= sizeof(rvlArchiveHeader));
  return (const u8*)((u8*)self + self->files.offset);
}

template <typename T> bool SafeMemCopy(T& dest, std::span<const u8> data) {
  assert(data.size_bytes() >= sizeof(T));
  if (data.size_bytes() < sizeof(T)) {
    std::fill((u8*)&dest, (u8*)(&dest + 1), 0);
    return false;
  }

  std::memcpy(&dest, data.data(), sizeof(T));
  return true;
}

bool RangeContains(std::span<const u8> range, const void* ptr) {
  return ptr >= range.data() && ptr < (range.data() + range.size());
}
bool RangeContainsInclusive(std::span<const u8> range, const void* ptr) {
  return ptr >= range.data() && ptr <= range.data() + range.size();
}

bool LoadU8Archive(LowU8Archive& result, std::span<const u8> data) {
  if (!SafeMemCopy(result.header, data))
    return false;

  const auto* nodes = rvlArchiveHeaderGetNodes(
      reinterpret_cast<const rvlArchiveHeader*>(data.data()));
  if (!RangeContains(data, nodes))
    return false;

  const auto node_count = nodes[0].folder.sibling_next;
  if (!RangeContainsInclusive(data, nodes + node_count))
    return false;

  result.nodes = {nodes, nodes + node_count};

  const char* strings = reinterpret_cast<const char*>(nodes + node_count);
  if (!RangeContains(data, strings))
    return false;

  const char* strings_end =
      reinterpret_cast<const char*>(nodes) + result.header.nodes.size;
  if (!RangeContainsInclusive(data, strings_end))
    return false;

  result.strings = {strings, strings_end};

  auto* fd_begin = rvlArchiveHeaderGetFileData(
      reinterpret_cast<const rvlArchiveHeader*>(data.data()));
  if (!RangeContains(data, fd_begin))
    return false;

  // For some reason the FD pointer is actually just the start of the file
  // fd_begin = data.data();
  int fd_trans = fd_begin - data.data();

  for (auto& node : result.nodes) {
    if (!rvlArchiveNodeIsFolder(node)) {
      node.file.offset = node.file.offset - fd_trans;
    }
  }

  result.file_data = {fd_begin, data.data() + data.size()};

  return true;
}

Result<U8Archive> LoadU8Archive(std::span<const u8> data) {
  U8Archive result;
  LowU8Archive low;
  if (!LoadU8Archive(low, data)) {
    return std::unexpected("Failed to read underlying data");
  }

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
      std::ofstream out(fpath);
      auto data = std::span(arc.file_data.begin(), arc.file_data.end())
                      .subspan(node.file.offset, node.file.size);
      EXPECT(data.size() == node.file.size);
      out.write(reinterpret_cast<const char*>(data.data()), data.size());
    }
  }
  return {};
}

} // namespace librii::U8
