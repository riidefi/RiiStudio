#include "Archive.hpp"

#include <librii/szs/SZS.hpp>
#include <librii/u8/U8.hpp>

IMPORT_STD;

std::optional<Archive> ReadArchive(std::span<const u8> buf,
                                   std::string& errcode) {
  auto expanded = librii::szs::getExpandedSize(buf);
  if (expanded == 0) {
    DebugReport("Failed to grab expanded size\n");
    errcode = "Invalid .szs file";
    return std::nullopt;
  }

  std::vector<u8> decoded(expanded);
  auto err = librii::szs::decode(decoded, buf);
  if (err) {
    DebugReport("Failed to decode SZS\n");
    errcode = "Invalid .szs file";
    return std::nullopt;
  }

  if (decoded.size() < 4 || decoded[0] != 0x55 || decoded[1] != 0xaa ||
      decoded[2] != 0x38 || decoded[3] != 0x2d) {
    DebugReport("Not a valid archive\n");
    errcode = "Not a U8 archive";
    return std::nullopt;
  }

  librii::U8::U8Archive arc;
  if (!librii::U8::LoadU8Archive(arc, decoded)) {
    DebugReport("Failed to read archive\n");
    errcode = "Invalid U8 archive";
    return std::nullopt;
  }

  Archive n_arc;

  struct Pair {
    Archive* folder;
    u32 sibling_next;
  };
  std::vector<Pair> n_path;

  assert(arc.nodes.size());

  n_path.push_back(
      Pair{.folder = &n_arc, .sibling_next = arc.nodes[0].folder.sibling_next});
  for (int i = 1; i < arc.nodes.size(); ++i) {
    auto& node = arc.nodes[i];

    while (!n_path.empty() && i == n_path.back().sibling_next)
      n_path.resize(n_path.size() - 1);

    if (node.is_folder) {
      auto tmp = std::make_unique<Archive>();
      n_path.push_back(
          Pair{.folder = tmp.get(), .sibling_next = node.folder.sibling_next});
      auto& parent = n_path[n_path.size() - 2];
      parent.folder->folders.emplace(node.name, std::move(tmp));
    } else {
      const u32 start_pos = node.file.offset;
      const u32 end_pos = node.file.offset + node.file.size;
      assert(node.file.offset + node.file.size <= arc.file_data.size());
      std::vector<u8> vec(arc.file_data.data() + start_pos,
                          arc.file_data.data() + end_pos);
      n_path.back().folder->files.emplace(node.name, std::move(vec));
    }

    while (!n_path.empty() && i + 1 == n_path.back().sibling_next)
      n_path.resize(n_path.size() - 1);
  }
  if (!n_path.empty()) {
    DebugReport("Invalid U8 structure\n");
    errcode = ".szs file was corrupted by BrawlBox";
    return std::nullopt;
  }

  // Eliminate the period
  if (!n_arc.folders.empty() && n_arc.folders.begin()->first == ".") {
    return *n_arc.folders["."];
  }

  return n_arc;
}

static void ProcessArcs(const Archive* arc, std::string_view name,
                        librii::U8::U8Archive& u8) {
  const auto node_index = u8.nodes.size();

  librii::U8::U8Archive::Node node{.is_folder = true,
                                   .name = std::string(name)};
  // Since we write folders first, the parent will always be behind us
  node.folder.parent = u8.nodes.size() - 1;
  node.folder.sibling_next = 0; // Filled in later
  u8.nodes.push_back(node);

  for (auto& data : arc->folders) {
    ProcessArcs(data.second.get(), data.first, u8);
  }

  for (auto& [n, f] : arc->files) {
    {
      librii::U8::U8Archive::Node node{.is_folder = false,
                                       .name = std::string(n)};
      node.file.offset =
          u8.file_data.size(); // Note: relative->abs translation handled later
      node.file.size = f.size();
      u8.nodes.push_back(node);
      u8.file_data.insert(u8.file_data.end(), f.begin(), f.end());
    }
  }

  u8.nodes[node_index].folder.sibling_next = u8.nodes.size();
}

std::vector<u8> WriteArchive(const Archive& arc) {
  librii::U8::U8Archive u8;
  u8.watermark = {0};

  ProcessArcs(&arc, ".", u8);

  auto u8_buf = librii::U8::SaveU8Archive(u8);
  auto szs_buf = librii::szs::encodeFast(u8_buf);

  return szs_buf;
}

std::optional<std::vector<u8>> FindFile(const Archive& arc, std::string path) {
  std::filesystem::path _path = path;
  _path = _path.lexically_normal();

  const Archive* cur_arc = &arc;
  for (auto&& part : _path) {
    {
      auto it = cur_arc->folders.find(part.string());
      if (it != cur_arc->folders.end()) {
        cur_arc = it->second.get();
        continue;
      }
    }
    {
      auto it = cur_arc->files.find(part.string());
      if (it != cur_arc->files.end()) {
        // TODO: This will ignore everything else in the path and accept invalid
        // item e.g. source/file.txt/invalid/other would ignore invalid/other
        return it->second;
      }
    }
  }

  return std::nullopt;
}

std::optional<ResolveQuery>
FindFileWithOverloads(const Archive& arc, std::vector<std::string> paths) {
  for (auto& path : paths) {
    auto found = FindFile(arc, path);
    if (found.has_value()) {
      return ResolveQuery{.file_data = std::move(*found),
                          .resolved_path = path};
    }
  }

  return std::nullopt;
}
