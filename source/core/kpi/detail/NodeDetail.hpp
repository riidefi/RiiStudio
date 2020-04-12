#pragma once

#include "../Node.hpp"
#include "../Reflection.hpp"

namespace kpi {

inline std::shared_ptr<const DocumentMemento>
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

inline void rollback(IDocumentNode& node,
                     std::shared_ptr<const DocumentMemento> record) {
  // Compare the actual data of the node
  if (!node.compareJustThisNotChildren(*record->JustData.get())) {
    node.fromData(*record->JustData.get());
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

namespace detail {

struct ApplicationPluginsImpl {
  // Requires TypeIdResolvable<T>, DefaultConstructible<T>
  template <typename T>
  struct TFactory final : public ApplicationPlugins::IFactory {
    std::unique_ptr<IFactory> clone() const override {
      return std::make_unique<TFactory<T>>(*this);
    }
    std::unique_ptr<IDocumentNode> spawn() override {
      return std::make_unique<TDocumentNode<T>>();
    }
    const char* getId() const override { return typeid(T).name(); }
  };
  //! Requires methods:
  //! - `T::canRead(const std::string& file, oishii::BinaryReader& reader)
  //! const`
  //! - `T::read(doc_node_t node, oishii::BinaryReader& reader) const`
  template <typename T>
  struct TBinaryDeserializer final : public IBinaryDeserializer, public T {
    std::unique_ptr<IBinaryDeserializer> clone() const override {
      return std::make_unique<TBinaryDeserializer<T>>(*this);
    }
    std::string canRead_(const std::string& file,
                         oishii::BinaryReader& reader) const override {
      return T::canRead(file, reader);
    }
    void read_(kpi::IDocumentNode& node,
               oishii::BinaryReader& reader) const override {
      T::read(node, reader);
    }
  };
  //! Requires methods:
  //! - `T::canWrite(doc_node_t node) const`
  //! - `T::write(doc_node_t node, oishii::v2::Writer& writer) const`
  template <typename T>
  struct TBinarySerializer final : public IBinarySerializer, public T {
    std::unique_ptr<IBinarySerializer> clone() const override {
      return std::make_unique<TBinarySerializer<T>>(*this);
    }
    bool canWrite_(kpi::IDocumentNode& node) const override {
      return T::canWrite(node);
    }
    void write_(kpi::IDocumentNode& node,
                oishii::v2::Writer& writer) const override {
      T::write(node, writer);
    }
  };
  //! Requires: `::write(doc_node_t, oishii::v2::Writer& writer, X*_=nullptr)`
  //! where `X` is some child that may be wrapped in a doc_node_t. No support
  //! for inheritance.
  template <typename T>
  struct TSimpleBinarySerializer final : public IBinarySerializer {
    std::unique_ptr<IBinarySerializer> clone() const override {
      return std::make_unique<TSimpleBinarySerializer<T>>(*this);
    }
    bool canWrite_(kpi::IDocumentNode& node) const override {
      return dynamic_cast<T*>(&node) != nullptr;
    }
    void write_(kpi::IDocumentNode& node,
                oishii::v2::Writer& writer) const override {
      write(node, writer, static_cast<T*>(nullptr));
    }
  };
};

} // namespace detail

template <typename T> inline ApplicationPlugins& ApplicationPlugins::addType() {
  mFactories[typeid(T).name()] =
      std::make_unique<detail::ApplicationPluginsImpl::TFactory<T>>();
  return *this;
}

template <typename T>
inline ApplicationPlugins& ApplicationPlugins::addSerializer() {
  mWriters.push_back(
      std::make_unique<detail::ApplicationPluginsImpl::TBinarySerializer<T>>());
  return *this;
}

template <typename T>
inline ApplicationPlugins& ApplicationPlugins::addSimpleSerializer() {
  mWriters.push_back(
      std::make_unique<
          detail::ApplicationPluginsImpl::TSimpleBinarySerializer<T>>());
  return *this;
}

template <typename T>
inline ApplicationPlugins& ApplicationPlugins::addDeserializer() {
  mReaders.push_back(std::make_unique<
                     detail::ApplicationPluginsImpl::TBinaryDeserializer<T>>());
  return *this;
}

inline void IDocumentNode::FolderData::add() {
  emplace_back(
      ApplicationPlugins::getInstance()->constructObject(type, parent));
}

inline bool IDocumentNode::FolderData::isSelected(std::size_t index) const {
  return std::find(state.selectedChildren.begin(), state.selectedChildren.end(),
                   index) != state.selectedChildren.end();
}

inline bool IDocumentNode::FolderData::select(std::size_t index) {
  if (isSelected(index))
    return true;

  state.selectedChildren.push_back(index);
  return false;
}

inline bool IDocumentNode::FolderData::deselect(std::size_t index) {
  auto it = std::find(state.selectedChildren.begin(),
                      state.selectedChildren.end(), index);

  if (it == state.selectedChildren.end())
    return false;
  state.selectedChildren.erase(it);
  return true;
}

inline std::size_t IDocumentNode::FolderData::clearSelection() {
  std::size_t before = state.selectedChildren.size();
  state.selectedChildren.clear();
  return before;
}

inline std::size_t IDocumentNode::FolderData::getActiveSelection() const {
  return state.activeSelectChild;
}

inline std::size_t
IDocumentNode::FolderData::setActiveSelection(std::size_t value) {
  const std::size_t old = state.activeSelectChild;
  state.activeSelectChild = value;
  return old;
}

} // namespace kpi
