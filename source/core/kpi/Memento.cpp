#include "Node.hpp"
#include "Reflection.hpp"

namespace kpi {

std::shared_ptr<const DocumentMemento>
setNext(const IDocumentNode& node,
        std::shared_ptr<const DocumentMemento> record) {
  DocumentMemento transient = *record.get();
  // Compare the actual data of the node
  if (record->JustData.get() == nullptr ||
      !node.compareJustThisNotChildren(*record->JustData.get())) {
    transient.JustData = std::make_shared<const IDocData>();
    transient.JustData = node.cloneDataNotChildren();
  }

  // Synchronize folders
  std::vector<std::string> new_folders, deleted_folders;
  std::set_difference(record->lut.begin(), record->lut.end(), node.lut.begin(),
                      node.lut.end(), std::back_inserter(deleted_folders));
  std::set_difference(node.lut.begin(), node.lut.end(), record->lut.begin(),
                      record->lut.end(), std::back_inserter(new_folders));

  for (const auto& folder : new_folders) {
    transient.children[folder];
    transient.lut.emplace(folder);
  }
  for (const auto& folder : deleted_folders) {
    transient.children.erase(folder);
    transient.lut.erase(folder);
  }

  // Now compare children
  for (const auto& folder : node.children) {
    auto& our_folder = transient.children[folder.first];

    // Ensure 1:1 mapping (TODO: Insertion is not handled gracefully)
    if (folder.second.size() != our_folder.size()) {
      our_folder.resize(folder.second.size());
      for (auto& p : our_folder)
        if (!p)
          p = std::make_shared<DocumentMemento>();
    }
    // Recurse to children
    for (std::size_t i = 0; i < folder.second.size(); ++i) {
      our_folder[i] = setNext(*folder.second[i].get(), our_folder[i]);
      // TODO: No parent ref count -- cannot orphan nodes!
    }
  }
  return transient == *record.get()
             ? record
             : std::make_shared<const DocumentMemento>(transient);
}

void rollback(IDocumentNode& node,
              std::shared_ptr<const DocumentMemento> record) {
  // Compare the actual data of the node
#ifdef BUILD_DEBUG
  auto cloned = node.cloneDataNotChildren();
  bool identity = node.compareJustThisNotChildren(*cloned.get());
  identity = node.compareJustThisNotChildren(*cloned.get());
  assert(identity);
#endif
  if (!node.compareJustThisNotChildren(*record->JustData.get())) {
    node.fromData(*record->JustData.get());
    node.onUpdate();
    assert(node.compareJustThisNotChildren(*record->JustData.get()));
  }

  // Synchronize folders
  std::vector<std::string> new_folders, deleted_folders;
  std::set_difference(record->lut.begin(), record->lut.end(), node.lut.begin(),
                      node.lut.end(), std::back_inserter(new_folders));
  std::set_difference(node.lut.begin(), node.lut.end(), record->lut.begin(),
                      record->lut.end(), std::back_inserter(deleted_folders));

  for (const auto& folder : new_folders) {
    node.children.insert({folder, {}});
    node.lut.emplace(folder);
  }
  for (const auto& folder : deleted_folders) {
    node.children.insert({folder, {}});
    node.lut.erase(folder);
  }

  // Now compare children
  for (const auto& folder : record->children) {
    auto& our_folder = node.children[folder.first];

    // Ensure 1:1 mapping (TODO: Insertion is not handled gracefully)
    if (folder.second.size() != our_folder.size()) {
      our_folder.resize(folder.second.size());
    }
    // Recurse to children
    for (std::size_t i = 0; i < folder.second.size(); ++i) {
      rollback(*our_folder[i].get(), folder.second[i]);
      our_folder[i]->parent = &node;
    }
  }
}

} // namespace kpi
