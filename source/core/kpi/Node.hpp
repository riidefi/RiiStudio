#pragma once

#include <algorithm>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include <core/common.h>

#include "History.hpp"
#include "Memento.hpp"
#include "Plugins.hpp"
#include "Reflection.hpp"

namespace kpi {

struct SelectionState {
  std::vector<std::size_t> selectedChildren;
  std::size_t activeSelectChild = 0;
};

struct IDocData {
  virtual ~IDocData() = default;
};

// The interface for view-only data.
// Referenced data is stored linearly in the folder-tree
class IDocumentNodeView {
public:
  virtual ~IDocumentNodeView() = default;

  virtual std::string getName() const { return "Untitled"; }
  std::string mType;
  IDocumentNodeView* parent = nullptr;

  // Generated. For example, tree structures based on relationships in the core
  // data.
  virtual std::vector<std::unique_ptr<IDocumentNodeView>> calcSimpleChildren() {
    return {};
  }
};

// we know use typeid for type
class IDocumentNode : public IDocumentNodeView {
public:
  virtual ~IDocumentNode() = default;

  virtual void fromData(const IDocData& rhs) = 0;
  virtual std::unique_ptr<IDocData> cloneDataNotChildren() const = 0;
  virtual std::unique_ptr<IDocumentNode> cloneDeep() const = 0;

  // Does not compare children
  virtual bool compareJustThisNotChildren(const IDocData& rhs) const = 0;

  // When changes to the node are undone/redone
  virtual void onUpdate() {}

  virtual std::string getName() const { return defaultNameField; }
  virtual void setName(const std::string& v) { defaultNameField = v; }
  std::string defaultNameField = "Untitled";

  struct FolderData : public std::vector<std::unique_ptr<IDocumentNode>> {
    FolderData() {}
    FolderData(const FolderData& rhs) { *this = rhs; }
    FolderData& operator=(const FolderData& rhs) {
      state = rhs.state;
      type = rhs.type;
      parent = rhs.parent;
      reserve(rhs.size());
      std::transform(rhs.begin(), rhs.end(), std::back_inserter(*this),
                     [](const std::unique_ptr<IDocumentNode>& it) {
                       return it->cloneDeep();
                     });
      return *this;
    }
    template <typename T> const T& at(std::size_t i) const {
      const T* as = dynamic_cast<const T*>(operator[](i).get());
      assert(as);
      return *as;
    }
    template <typename T> T& at(std::size_t i) {
      T* as = dynamic_cast<T*>(operator[](i).get());
      assert(as);
      return *as;
    }
    //! @brief Construct a new node of the folder type and append it. The folder
    //! type *must* be constructible.
    //!
    void add();

    //! @brief Return if a node is selected at the specified index.
    //!
    bool isSelected(std::size_t index) const;

    //! @brief Select a node at the specified index.
    //!
    bool select(std::size_t index);

    //! @brief Deselect a node at the specified index.
    //!
    bool deselect(std::size_t index);

    //! @brief Clear the selection. Note: Active selection will not change.
    //!
    //! @return The number of selections prior to clearing.
    //!
    std::size_t clearSelection();

    //! @brief Return the active selection index.
    //!
    std::size_t getActiveSelection() const;

    //! @brief Set the active selection index.
    //!
    //! @param[in] value The new index to use.
    //!
    //! @return The last active selection index.
    //!
    std::size_t setActiveSelection(std::size_t value);

    SelectionState state;
    std::string type;
    IDocumentNode* parent;
  };
  const FolderData* getFolder(const std::string& type, bool fromParent = false,
                              bool fromChild = false) const {
    for (auto& c : children)
      if (c.first == type)
        return &c.second;

    auto info = kpi::ReflectionMesh::getInstance()->lookupInfo(type);

    if (!fromChild) {
      for (int i = 0; i < info.getNumParents(); ++i) {
        assert(info.getParent(i).getName() != info.getName());

        auto* opt = getFolder(info.getParent(i).getName(), true, false);
        if (opt != nullptr)
          return opt;
      }
    }

    if (!fromParent) {
      // If the folder is of a more specialized type
      for (int i = 0; i < info.getNumChildren(); ++i) {
        const std::string childName = info.getChild(i).getName();
        const std::string infName = info.getName();
        assert(childName != infName);

        auto* opt = getFolder(childName, false, true);
        if (opt != nullptr)
          return opt;
      }
    }

    return {};
  }

  template <typename T> const FolderData* getFolder() const {
    return getFolder(typeid(T).name());
  }
  template <typename T> FolderData* getFolder() {
    return const_cast<FolderData*>(getFolder(typeid(T).name()));
  }
  // Do not call without ensuring a folder does not already exist.
  FolderData* addFolder(const std::string& type) {
    children.emplace(type, FolderData{});
    lut.emplace(type);
    return &children[type];
  }
  template <typename T> FolderData* addFolder() {
    return addFolder(typeid(T).name());
  }

  FolderData& getOrAddFolder(const std::string& type) {
    auto* f = const_cast<FolderData*>(getFolder(type));
    if (f == nullptr)
      f = addFolder(type);
    assert(f);
    f->type = type;
    f->parent = this;
    return *f;
  }
  template <typename T> FolderData& getOrAddFolder() {
    return getOrAddFolder(typeid(T).name());
  }

  // SelectionState select;

  std::map<std::string, FolderData> children;
  std::set<std::string> lut;
};

using FolderData = IDocumentNode::FolderData;

template <typename T> struct TDocData : public IDocData, T {
  TDocData(const T& dr) : T(dr) {}
};
// Might be huge, say a vertex array -- and likely never changed!
template <typename T>
struct TDocumentNode final : public IDocumentNode, public T {
  TDocumentNode() = default;
  // Potential overrides: getParent, getChild, getNextSibling, etc
  const IDocumentNode* getParent() const {
    assert(dynamic_cast<const IDocumentNode*>(parent) != nullptr);
    return static_cast<const IDocumentNode*>(parent);
  }
  IDocumentNode* getParent() {
    assert(dynamic_cast<IDocumentNode*>(parent) != nullptr);
    return static_cast<IDocumentNode*>(parent);
  }

  template <typename Q> class has_get_name {
    typedef char YesType[1];
    typedef char NoType[2];

    template <typename C> static YesType& test(decltype(&C::getName));
    template <typename C> static NoType& test(...);

  public:
    enum { value = sizeof(test<Q>(0)) == sizeof(YesType) };
  };
  template <typename Q> class has_on_update {
    typedef char YesType[1];
    typedef char NoType[2];

    template <typename C> static YesType& test(decltype(&C::onUpdate));
    template <typename C> static NoType& test(...);

  public:
    enum { value = sizeof(test<Q>(0)) == sizeof(YesType) };
  };

  struct TestA {
    std::string getName() { return ""; }
  };
  struct TestB {};

  static_assert(has_get_name<TestA>::value, "Detecting getName");
  static_assert(!has_get_name<TestB>::value, "Detecting getName");

  template <bool B> std::string _getName() const { return defaultNameField; }
  template <> std::string _getName<true>() const { return T::getName(); }

  std::string getName() const override {
    return _getName<has_get_name<T>::value>();
  }

  //	void setName(const std::string& v) override {
  //		// _setName(v);
  //	}

  template <bool B> void _onUpdate() {}
  template <> void _onUpdate<true>() { T::onUpdate(); }

  void onUpdate() override { _onUpdate<has_on_update<T>::value>(); }

  // Does not copy anything but data
  void fromData(const IDocData& rhs) override {
    const T* pdat = dynamic_cast<const T*>(&rhs);
    assert(pdat);
    if (pdat)
      *static_cast<T*>(this) = *pdat;
  }
  std::unique_ptr<IDocData> cloneDataNotChildren() const override {
    const auto& dr = *static_cast<const T*>(this);
    return std::make_unique<TDocData<T>>(dr);
  }
  std::unique_ptr<IDocumentNode> cloneDeep() const override {
    return std::make_unique<TDocumentNode<T>>(*this);
  }
  bool compareJustThisNotChildren(const IDocData& rhs) const override {
    const T* pdat = dynamic_cast<const T*>(&rhs);
    assert(pdat != nullptr);
    return pdat && *pdat == *static_cast<const T*>(this);
  }
};

template <typename T> class NodeAccessor {
public:
  T& get() {
    assert(data);
    return *data;
  }
  const T& get() const {
    assert(data);
    return *data;
  }
  IDocumentNode& node() {
    assert(data);
    return *data;
  }
  const IDocumentNode& node() const {
    assert(data);
    return *data;
  }
  bool valid() const { return data != nullptr; }

  NodeAccessor(IDocumentNode* node) { setInternal(node); }
  NodeAccessor(IDocumentNode& node) { setInternal(node); }
  void setInternal(IDocumentNode* node) {
    if (node == nullptr) {
      data = nullptr;
    } else {
      assert(dynamic_cast<TDocumentNode<T>*>(node) != nullptr);
      data = reinterpret_cast<TDocumentNode<T>*>(node);
    }
  }
  void setInternal(IDocumentNode& node) {
    assert(dynamic_cast<TDocumentNode<T>*>(&node) != nullptr);
    data = reinterpret_cast<TDocumentNode<T>*>(&node);
  }
  operator T&() { return get(); }
  operator const T&() const { return get(); }

#define __KPI_FMT_NODE(type) get##type##s
#define KPI_NODE_FOLDER(type, acc)                                             \
  kpi::FolderData& __KPI_FMT_NODE(type)() {                                    \
    auto* f = data->getFolder<type>();                                         \
    assert(f);                                                                 \
    return *f;                                                                 \
  }                                                                            \
  const kpi::FolderData& __KPI_FMT_NODE(type)() const {                        \
    auto* f = data->getFolder<type>();                                         \
    assert(f);                                                                 \
    return *f;                                                                 \
  }                                                                            \
  type& get##type##Raw(std::size_t x) {                                        \
    auto* f = data->getFolder<type>();                                         \
    assert(f);                                                                 \
    return f->at<type>(x);                                                     \
  }                                                                            \
  const type& get##type##Raw(std::size_t x) const {                            \
    auto* f = data->getFolder<type>();                                         \
    assert(f);                                                                 \
    return f->at<type>(x);                                                     \
  }                                                                            \
  acc get##type(std::size_t x) {                                               \
    auto* f = data->getFolder<type>();                                         \
    assert(f);                                                                 \
    return {f->at<kpi::IDocumentNode>(x)};                                     \
  }                                                                            \
  const acc get##type(std::size_t x) const {                                   \
    auto* f = data->getFolder<type>();                                         \
    assert(f);                                                                 \
    return {f->at<kpi::IDocumentNode>(x)};                                     \
  }                                                                            \
  type& add##type##Raw() {                                                     \
    auto& f = data->getOrAddFolder<type>();                                    \
    f.add();                                                                   \
    return f.at<type>(f.size() - 1);                                           \
  }                                                                            \
  acc add##type() {                                                            \
    auto& f = data->getOrAddFolder<type>();                                    \
    f.add();                                                                   \
    return {f.at<kpi::IDocumentNode>(f.size() - 1)};                           \
  }

#define KPI_NODE_FOLDER_SIMPLE(type)                                           \
  KPI_NODE_FOLDER(type, kpi::NodeAccessor<type>)

protected:
  TDocumentNode<T>* data = nullptr;
};

} // namespace kpi

#include "detail/NodeDetail.hpp"
