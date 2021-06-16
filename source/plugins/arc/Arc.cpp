#ifndef __linux__

#include "Arc.hpp"

namespace riistudio::arc {

bool Archive::exists(const Path& path) const {
  return findFstNode(path) != fstNodeSentinel();
}

bool Archive::isFolder(const Path& path) const {
  return getEntry(path).isFolder();
}
bool Archive::isFile(const Path& path) const { return getEntry(path).isFile(); }

void Archive::deleteEntry(const Path& path) {
  auto found = findFstNode(path);
  if (found == fstNodeSentinel())
    return;

  auto& entry = found->second;

  orphanFstNode(path);

  // Delete children
  if (entry.isFolder()) {
  hack:
    for (auto& child : entry.getChildren()) {
      deleteEntry(std::filesystem::path(child.c_str()));
      goto hack;
    }
  }

  eraseFstEntry(path);
}

kpi::IMementoOriginator* Archive::getFile(const Path& path) {
  const Archive* cthis = this;
  return const_cast<kpi::IMementoOriginator*>(cthis->getFile(path));
}
const kpi::IMementoOriginator* Archive::getFile(const Path& path) const {
  auto found = findFstNode(path);
  if (found == fstNodeSentinel())
    return nullptr;

  auto& entry = found->second;
  if (entry.isFolder())
    return nullptr;

  return entry.getFileData();
}

void Archive::setFile(const Path& path,
                      std::unique_ptr<kpi::IMementoOriginator>&& data) {
  auto found = findFstNode(path);
  assert(found != fstNodeSentinel());
  auto& entry = found->second;
  assert(!entry.isFolder());
  return entry.setFileData(std::move(data));
}

bool Archive::createFolder(const Path& _path) {
  auto fst_node = findFstNode(_path);
  if (fst_node != fstNodeSentinel())
    return fst_node->second.isFolder();

  Path path = _path.lexically_normal();
  Path building;
  for (auto part_it = path.begin(); part_it != path.end(); ++part_it) {
    building /= *part_it;
    if (exists(building)) {
      if (!isFolder(building))
        return false;

      continue;
    }

    FstNode info(EntryType::Folder);

    auto next_part = part_it;
    ++next_part;
    if (next_part != path.end())
      info.addChild(building / *next_part);

    emplaceFstNode(building, std::move(info));
  }

  return true;
}

bool Archive::createFile(const Path& path,
                         std::unique_ptr<kpi::IMementoOriginator> data) {
  auto fst_node = findFstNode(path);
  if (fst_node != fstNodeSentinel())
    return fst_node->second.isFile();

  if (path.has_parent_path()) {
    const auto parent_path = path.parent_path();
    if (!exists(parent_path))
      createFolder(parent_path);
    getEntry(parent_path).addChild(path);
  }

  emplaceFstNode(path, std::move(data));
  return true;
}

const std::set<NormalizedPath>&
Archive::getFolderChildren(const Path& path) const {
  assert(exists(path) && isFolder(path));
  return getEntry(path).getChildren();
}

const Archive::FstNode& Archive::getEntry(const Path& path) const {
  assert(exists(path));
  return findFstNode(path)->second;
}

Archive::FstNode& Archive::getEntry(const Path& path) {
  assert(exists(path));
  return findFstNode(path)->second;
}

} // namespace riistudio::arc

#endif
