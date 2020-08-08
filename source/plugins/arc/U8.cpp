#include "Arc.hpp"
#include <core/common.h>

#include <span>
#include <string_view>

#include <oishii/reader/stream_raii.hpp>

#include <core/api.hpp>

namespace riistudio::arc::u8 {

static std::unique_ptr<kpi::IMementoOriginator>
constructFile(const std::string_view path, std::span<const uint8_t> data) {
  const auto construct_node = [&]() -> std::unique_ptr<kpi::INode> {
    if (data.empty())
      return nullptr;
    try {
      oishii::DataProvider provider({data.begin(), data.end()}, path);
      auto importer = SpawnImporter(std::string(path), provider.slice());

      // Don't worry about ambiguous cases for now
      if (!importer.second || !IsConstructible(importer.first))
        return nullptr;

      std::unique_ptr<kpi::INode> fileState{
          dynamic_cast<kpi::INode*>(SpawnState(importer.first).release())};
      if (!fileState.get())
        return nullptr;

      kpi::IOTransaction transaction{*fileState, provider.slice(), [](...) {}};
      importer.second->read_(transaction);

      return fileState;
    } catch (const char* mesg) {
      return nullptr;
    }
  };

  if (auto node = construct_node(); node) {
    return node;
  }
  return std::make_unique<RawBinaryOriginator>(data);
}

void readArchive(Archive& dst, oishii::BinaryReader& reader) {
  const auto start = reader.tell();
  const auto* start_ptr = reader.getStreamStart() + start;

  reader.skip(4); // skip magic
  const auto fst_start = reader.read<s32>();
  MAYBE_UNUSED const auto fst_size = reader.read<s32>();
  MAYBE_UNUSED const auto data_ofs = reader.read<s32>();
  reader.skip(16); // pad

  const auto strings_offset =
      fst_start + reader.getAt<u32>(start + fst_start + 8) * 12;

  const char* strings =
      reinterpret_cast<const char*>(start_ptr) + strings_offset;

  const auto get_name = [&](u32 offset) -> std::string_view {
    u32 end = offset;
    while (strings[end] != 0)
      ++end;
    return std::string_view(strings + offset, end - offset);
  };
  const auto get_data = [&](u32 offset, u32 size) -> std::span<const uint8_t> {
    return {start_ptr + offset, size};
  };

  struct Folder {
    u32 parent_id;
    u32 sibilng_next;
  };
  struct Node {
    std::string_view name;
    // union {
    std::span<const uint8_t> file;
    Folder folder;
    // };
    bool isFolder = false;
    std::span<const uint8_t>* asFile() { return isFolder ? nullptr : &file; }
    Folder* asFolder() { return isFolder ? &folder : nullptr; }
  };

  const auto read_entry = [&](u32 idx) -> Node {
    oishii::Jump<oishii::Whence::Set> g(reader, start + fst_start + 12 * idx);

    const auto type_name = reader.read<u32>();
    const auto type = type_name >> 24;
    const auto name = type_name & 0xff'ffff;

    const auto parent_pos = reader.read<u32>();
    const auto sibling_size = reader.read<u32>();

    Node result;
    result.name = get_name(name);
    result.isFolder = type == 1;
    if (type == 1) {
      result.folder = Folder{parent_pos, sibling_size};
    } else {
      DebugReport("Reading file %s. Pos:%u Size:%u\n", result.name.data(),
                  parent_pos, sibling_size);
      result.file = get_data(parent_pos, sibling_size);
    }

    return result;
  };

  Node root = read_entry(0);
  assert(root.asFolder() != nullptr);

  // Folder -> Trace up parents
  // File -> Trace left until folder
  const auto create_path = [&](u32 index) -> std::filesystem::path {
    std::vector<std::string> tmp;
    Node node = read_entry(index);
    DebugReport("\nCreating path for %s\n", node.name.data());
    if (auto* file = node.asFile(); file != nullptr) {
      tmp.emplace_back(node.name);
      DebugReport("File.. trace left\n");
      for (u32 trace = index - 1; trace > 0; --trace) {
        auto sibling = read_entry(trace);
        DebugReport("Considering \"%s\"\n", sibling.name.data());
        if (sibling.asFolder() != nullptr) {
          DebugReport("Anchored. Trace up.\n");
          node = sibling;
          index = trace;
          break;
        }
      }
      // If trace == 0, the file is a child of the root
    }
    if (node.asFolder() != nullptr) {
      auto folder = *node.asFolder();
      while (folder.parent_id != 0) {
        tmp.emplace_back(read_entry(index).name);
        index = folder.parent_id;
        folder = *read_entry(index).asFolder();
      }
    }
    tmp.emplace_back(root.name);
    std::reverse(tmp.begin(), tmp.end());
    std::filesystem::path result;
    for (auto& part : tmp)
      result /= part;
    return result;
  };

  for (u32 i = 1; i < root.asFolder()->sibilng_next; ++i) {
    Node entry = read_entry(i);
    const auto path = create_path(i);
    if (auto* file = entry.asFile(); file != nullptr) {
      DebugReport("Unpacking file %s\n", path.string().c_str());
      dst.createFile(path, constructFile(path.string(), *file));
    }
    // Necessary for empty folders
    if (auto* folder = entry.asFolder(); folder != nullptr) {
      dst.createFolder(path);
    }
  }
}

} // namespace riistudio::arc::u8
