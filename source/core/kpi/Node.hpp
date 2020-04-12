#pragma once

#include <algorithm>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include <core/common.h>

#include "Reflection.hpp"

namespace oishii {
class BinaryReader;
namespace v2 {
class Writer;
}
} // namespace oishii

struct SelectionState {
  std::vector<std::size_t> selectedChildren;
  std::size_t activeSelectChild = 0;
};

namespace kpi {

template <typename T> using const_shared_ptr = std::shared_ptr<const T>;

struct IDocData {
  virtual ~IDocData() = default;
};

// we know use typeid for type
class IDocumentNode {
public:
  virtual ~IDocumentNode() = default;

  virtual void fromData(const IDocData& rhs) = 0;
  virtual std::unique_ptr<IDocData> cloneDataNotChildren() const = 0;
  virtual std::unique_ptr<IDocumentNode> cloneDeep() const = 0;

  // Does not compare children
  virtual bool compareJustThisNotChildren(const IDocData& rhs) const = 0;

  virtual std::string getName() const { return defaultNameField; }
  virtual void setName(const std::string& v) { defaultNameField = v; }
  std::string mType;
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

  SelectionState select;
  IDocumentNode* parent = nullptr;
  std::map<std::string, FolderData> children;
  std::set<std::string> lut;
};

struct DocumentMemento {
  std::shared_ptr<const DocumentMemento> parent = nullptr;
  std::map<std::string, std::vector<std::shared_ptr<const DocumentMemento>>>
      children;
  std::set<std::string> lut;
  std::shared_ptr<const IDocData> JustData = nullptr;

  bool operator==(const DocumentMemento& rhs) const {
    return parent == rhs.parent && children == rhs.children && lut == rhs.lut &&
           JustData == rhs.JustData;
  }
};

// Permute a persistent immutable document record, sharing memory where possible
std::shared_ptr<const DocumentMemento>
setNext(const IDocumentNode& node,
        std::shared_ptr<const DocumentMemento> record);
// Restore a transient document node to a recorded state.
void rollback(IDocumentNode& node,
              std::shared_ptr<const DocumentMemento> record);

using FolderData = IDocumentNode::FolderData;

template <typename T> struct TDocData : public IDocData, T {
  TDocData(const T& dr) : T(dr) {}
};
// Might be huge, say a vertex array -- and likely never changed!
template <typename T>
struct TDocumentNode final : public IDocumentNode, public T {
  TDocumentNode() = default;
  // Potential overrides: getParent, getChild, getNextSibling, etc
  const IDocumentNode* getParent() const { return parent; }
  IDocumentNode* getParent() { return parent; }

  template <typename Q> class has_get_name {
    typedef char YesType[1];
    typedef char NoType[2];

    template <typename C> static YesType& test(decltype(&C::getName));
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

/*
Application:
        - Type hierarchy
        - Factories: Construct type
        - Serializers: Write/Read constructed type
*/

//! A reader: Do not inherit from this type directly
struct IBinaryDeserializer {
  virtual ~IBinaryDeserializer() = default;
  virtual std::unique_ptr<IBinaryDeserializer> clone() const = 0;
  virtual std::string canRead_(const std::string& file,
                               oishii::BinaryReader& reader) const = 0;
  virtual void read_(kpi::IDocumentNode& node,
                     oishii::BinaryReader& reader) const = 0;
};
//! A writer: Do not inherit from this type directly
struct IBinarySerializer {
  virtual ~IBinarySerializer() = default;
  virtual std::unique_ptr<IBinarySerializer> clone() const = 0;
  virtual bool canWrite_(kpi::IDocumentNode& node) const = 0;
  virtual void write_(kpi::IDocumentNode& node,
                      oishii::v2::Writer& writer) const = 0;
};

// Part of the application state itself. Not part of the persistent document.
class ApplicationPlugins {
  friend class ApplicationPluginsImpl;

public:
  //! @brief Add a type to the internal registry for future construction and
  //! manipulation.
  //!
  //! @tparam T Any default constructible type.
  //!
  //! @details Example:
  //!				struct SomeType
  //!				{
  //!					SomeType() = default;
  //!					int value = 0;
  //!				};
  //!
  //!				ApplicationPlugins::getInstance()->addType<SomeType>();
  //!
  template <typename T> ApplicationPlugins& addType();

  //! @brief Add a type with a parent.
  //!
  template <typename T, typename P> inline ApplicationPlugins& addType() {
    addType<T>();
    registerParent<T, P>();
    return *this;
  }

  //! @brief Add a binary serializer (writer) to the internal registry.
  //!
  //! @tparam T Any default constructible type T with member functions:
  //!				- `bool T::canWrite(doc_node_t node) const`
  //!(return if we can write)
  //!				- `T::write(doc_node_t node, oishii::v2::Writer&
  //! writer) const` (write the file)
  //!
  //! @details Example:
  //!				struct SomeWriter
  //!				{
  //!					bool canWrite(doc_node_t node) const
  //!					{
  //!						// If `node` is not of our type
  //!`SomeType` from before or a child of it, we cannot write it.
  //! const SomeType* data = dynamic_cast<const SomeType*>(node.get());
  //! if (data == nullptr) { return false;
  //!						}
  //!
  //!						// For example, we might not be
  //! able to express negative
  //! numbers in this format. 						if
  //! (data->value < 0) { return false;
  //!						}
  //!
  //!						return true;
  //!					}
  //!					void write(doc_node_t node,
  //! oishii::v2::Writer& writer) const
  //!					{
  //!						// We will not be here unless
  //! our last check
  //! passed. 						const SomeType* data =
  //! dynamic_cast<const SomeType*>(node.get());
  //! assert(data != nullptr);
  //!
  //!						// Write our file magic /
  //! identifier. writer.write<u8>('S'); writer.write<u8>('O');
  //! writer.write<u8>('M');
  //! writer.write<u8>('E');
  //!						// Write our data
  //!						writer.write<s32>(data->value);
  //!					}
  //!				};
  //!
  //!				ApplicationPlugins::getInstance()->addSerializer<SomeWriter>();
  //!
  template <typename T> ApplicationPlugins& addSerializer();

  //! @brief Add a binary serializer (writer) to the internal registry with a
  //! simplified API.
  //!
  //! @tparam T Any default constructible type T where the following function
  //! exists:
  //!				- `::write(doc_node_t, oishii::v2::Writer&
  //! writer, X*_=nullptr)` where `X` is some child that may be wrapped in a
  //! doc_node_t.
  //!
  //! @details Example:
  //!				// `dummy` is necessary to distinguish this
  //!`write` function from other `write` functions.
  //!				// It will always be nullptr.
  //!				void write(doc_node_t node, oishii::v2::Writer&
  //! writer, SomeType* dummy=nullptr) const
  //!				{
  //!					// We will not be here unless node is a
  //! child of SomeType or SomeType itself. const
  //! SomeType*
  //! data = dynamic_cast<const
  //! SomeType*>(node.get()); 					assert(data !=
  //! nullptr);
  //!
  //!					// Write our file magic / identifier.
  //!					writer.write<u8>('S');
  //!					writer.write<u8>('O');
  //!					writer.write<u8>('M');
  //!					writer.write<u8>('E');
  //!					// Write our data
  //!					writer.write<s32>(data->value);
  //!				}
  //!
  //!				ApplicationPlugins::getInstance()->addSimpleSerializer<SomeType>();
  //!
  template <typename T> ApplicationPlugins& addSimpleSerializer();

  //! @brief Add a binary deserializer (reader) to the internal registry.
  //!
  //! @tparam T Any default constructible type T with member functions:
  //!				- `std::string T::canRead(const std::string&
  //! file, oishii::BinaryReader& reader) const` (return the type of the file
  //! identified or empty)
  //!				- `T::read(doc_node_t node,
  //! oishii::BinaryReader& reader) const` (read the file)
  //!
  //! @details Example:
  //!				struct SomeReader
  //!				{
  //!					std::string canRead(const std::string&
  //! file, oishii::BinaryReader& reader) const
  //!					{
  //!						// Our file is eight bytes long.
  //!						if (reader.endpos() != 8) {
  //!							return "";
  //!						}
  //!
  //!						// Our file uses the file magic
  //!/ identifier 'SOME'. 						if
  //!(reader.peek<u8>(0) != 'S' ||
  //! reader.peek<u8>(1)
  //!!= 'O' || reader.peek<u8>(2) != 'M'
  //!|| reader.peek<u8>(3)
  //!!= 'E') { return "";
  //!						}
  //!
  //!						// Use the name of SomeType to
  //! identify it later. 						return
  //! typeid(SomeType).name;
  //!					}
  //!					void read(doc_node_t node,
  //! oishii::BinaryReader& writer) const
  //!					{
  //!						// Ensure that the constructed
  //! document node is derived
  //! from or of type SomeType. const SomeType* data = dynamic_cast<const
  //! SomeType*>(node.get()); assert(data != nullptr);
  //!
  //!						// We already checked our magic
  //! in our `canRead` function. We can ignore it.
  //! reader.seek(4);
  //!
  //!						// Read our data
  //!						data->value =
  //! reader.read<s32>();
  //!					}
  //!				};
  //!
  //!				ApplicationPlugins::getInstance()->addDeserializer<SomeReader>();
  //!
  template <typename T> ApplicationPlugins& addDeserializer();

  virtual void registerMirror(const kpi::MirrorEntry& entry) = 0;

  template <typename D, typename B> ApplicationPlugins& registerParent() {
    registerMirror(
        {typeid(D).name(), typeid(B).name(), computeTranslation<D, B>()});
    return *this;
  }
  template <typename D, typename B>
  ApplicationPlugins& registerMember(int slide) {
    registerMirror({typeid(D).name(), typeid(B).name(), slide});
    return *this;
  }

  virtual void installModule(const std::string& path) = 0;

  virtual std::unique_ptr<kpi::IDocumentNode>
  constructObject(const std::string& type,
                  kpi::IDocumentNode* parent = nullptr) const = 0;

  static inline ApplicationPlugins* getInstance() { return spInstance; }
  virtual ~ApplicationPlugins() = default;

public:
  static ApplicationPlugins* spInstance;

  struct IFactory {
    virtual ~IFactory() = default;
    virtual std::unique_ptr<IFactory> clone() const = 0;
    virtual std::unique_ptr<IDocumentNode> spawn() = 0;
    virtual const char* getId() const = 0;
  };

  std::map<std::string, std::unique_ptr<IFactory>> mFactories;
  std::vector<std::unique_ptr<IBinaryDeserializer>> mReaders;
  std::vector<std::unique_ptr<IBinarySerializer>> mWriters;
};

class History {
public:
  void commit(const IDocumentNode& doc) {
    if (history_cursor >= 0)
      root_history.erase(root_history.begin() + history_cursor + 1,
                         root_history.end());
    root_history.push_back(setNext(
        doc, root_history.empty() ? std::make_shared<const DocumentMemento>()
                                  : root_history.back()));
    ++history_cursor;
  }
  void undo(IDocumentNode& doc) {
    if (history_cursor <= 0)
      return;
    --history_cursor;
    rollback(doc, root_history[history_cursor]);
  }
  void redo(IDocumentNode& doc) {
    if (history_cursor + 1 >= root_history.size())
      return;
    ++history_cursor;
    rollback(doc, root_history[history_cursor]);
  }
  std::size_t cursor() const { return history_cursor; }
  std::size_t size() const { return root_history.size(); }

private:
  // At the roots, we don't need persistence
  // We don't ever expose history to anyone -- only the current document
  std::vector<std::shared_ptr<const DocumentMemento>> root_history;
  int history_cursor = -1;
};

} // namespace kpi

#include "detail/NodeDetail.hpp"
