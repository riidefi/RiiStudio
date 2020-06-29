#include "Node.hpp"
#include "Reflection.hpp"

namespace kpi {

std::shared_ptr<const DocumentMemento>
setNext(const INode& node, std::shared_ptr<const DocumentMemento> record) {
#if 0
  DocumentMemento transient = *record.get();
  // Compare the actual data of the node
  auto* old_data = record->JustData.get();
  auto* new_data = node.getImmediateData();

  if (new_data != nullptr &&
      (old_data == nullptr || !(*new_data == *old_data))) {
    transient.JustData = std::make_shared<const IDocData>();
    transient.JustData = node.getImmediateData()->clone();
  }

  // Synchronize folders
  std::vector<std::string> node_folders(node.numFolders());
  for (int i = 0; i < node.numFolders(); ++i) {
    node_folders.emplace_back(node.idAt(i));
  }

  std::vector<std::string> new_folders, deleted_folders;
  std::set_difference(record->lut.begin(), record->lut.end(),
                      node_folders.begin(), node_folders.end(),
                      std::back_inserter(deleted_folders));
  std::set_difference(node_folders.begin(), node_folders.end(),
                      record->lut.begin(), record->lut.end(),
                      std::back_inserter(new_folders));

  for (const auto& folder : new_folders) {
    transient.children[folder];
    transient.lut.emplace(folder);
  }
  for (const auto& folder : deleted_folders) {
    transient.children.erase(folder);
    transient.lut.erase(folder);
  }

  // Now compare children
  for (int i = 0; i < node.numFolders(); ++i) {
    const auto& folder = *node.folderAt(i);
    auto& our_folder = transient.children[node.idAt(i)];

    // Ensure 1:1 mapping (TODO: Insertion is not handled gracefully)
    if (folder.size() != our_folder.size()) {
      our_folder.resize(folder.size());
      for (auto& p : our_folder)
        if (!p)
          p = std::make_shared<DocumentMemento>();
    }
    // Recurse to children
    for (std::size_t i = 0; i < folder.size(); ++i) {
      our_folder[i] = setNext(*folder.at(i), our_folder[i]);
      // TODO: No parent ref count -- cannot orphan nodes!
    }
  }
  return transient == *record.get()
             ? record
             : std::make_shared<const DocumentMemento>(transient);
#endif
  return nullptr;
}

void rollback(INode& node, std::shared_ptr<const DocumentMemento> record) {
#if 0
  // Compare the actual data of the node
#ifdef BUILD_DEBUG
  auto cloned = node.getImmediateData()->clone();
  bool identity = *cloned == *node.getImmediateData();
  identity = *cloned == *node.getImmediateData();
  assert(identity);
#endif
  if (!(*node.getImmediateData() == *record->JustData.get())) {
    *node.getImmediateData() = *record->JustData.get();
    assert(*node.getImmediateData() == *record->JustData.get());
  }

  // Synchronize folders
  std::vector<std::string> node_folders(node.numFolders());
  for (int i = 0; i < node.numFolders(); ++i) {
    node_folders.emplace_back(node.idAt(i));
  }

  // Not possible to sync folders back.. cannot construct arbitrary types.
  // We'd need to maintain some factory handle.. perhaps all INodes should be
  // constructible

#if 0
  std::vector<std::string> new_folders, deleted_folders;
  std::set_difference(record->lut.begin(), record->lut.end(),
                      node_folders.begin(), node_folders.end(),
                      std::back_inserter(new_folders));
  std::set_difference(node_folders.begin(), node_folders.end(),
                      record->lut.begin(), record->lut.end(),
                      std::back_inserter(deleted_folders));

  for (const auto& folder : new_folders) {
    node.children.insert({folder, {}});
    node.lut.emplace(folder);
  }
  for (const auto& folder : deleted_folders) {
    node.children.insert({folder, {}});
    node.lut.erase(folder);
  }
#endif

  // Now compare children
  for (const auto& folder : record->children) {
    const auto idx = node.fromId(folder.first.c_str());
    assert(idx != ~0);
    auto& our_folder = *node.folderAt(idx);

    // Ensure 1:1 mapping (TODO: Insertion is not handled gracefully)
    if (folder.second.size() != our_folder.size()) {
      our_folder.resize(folder.second.size());
    }
    // Recurse to children
    for (std::size_t i = 0; i < folder.second.size(); ++i) {
      auto* as_node = dynamic_cast<kpi::INode*>(our_folder.atObject(i));
      if (as_node != nullptr)
        rollback(*as_node, folder.second[i]);
      else
        *our_folder.setAt(i) = *folder.second[i]->JustData().get();
      our_folder.atObject(i)->childOf = &node;
      our_folder.atObject(i)->collectionOf = &our_folder;
    }
  }
#endif
}

} // namespace kpi
