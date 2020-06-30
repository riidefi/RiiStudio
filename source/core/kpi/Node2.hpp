// Second Node architecture

#pragma once

#include <algorithm>              // std::find_if
#include <core/common.h>          // u32
#include <cstddef>                // std::size_t
#include <llvm/ADT/SmallVector.h> // llvm::SmallVector
#include <string_view>            // std::string_view
#include <type_traits>            // std::is_same_v
#include <vector>                 // std::vector

namespace kpi {

struct ICollection;
struct INode;

// INode -- Owner of folders
// IObject -- Folder item
struct IObject {
  virtual ~IObject() = default;

  virtual std::string getName() const { return "TODO"; }

  // For now, at least, all objects exist in collections
  ICollection* collectionOf = nullptr;
  // The owner of the collection
  INode* childOf = nullptr;
};
struct SelectionState {
  std::vector<std::size_t> selectedChildren;
  std::size_t activeSelectChild = 0;
};

struct ICollection {
  virtual ~ICollection() = default;
  virtual std::size_t size() const = 0;
  // Collection of type T, return (void*)(T*)
  virtual void* at(std::size_t) = 0;
  virtual const void* at(std::size_t) const = 0;
  virtual void resize(std::size_t) = 0;
  // Class composed of T
  virtual IObject* atObject(std::size_t) = 0;
  virtual const IObject* atObject(std::size_t) const = 0;
  virtual void add() = 0;

  std::size_t indexOf(const std::string_view name) const {
    const auto _size = size();
    for (std::size_t i = 0; i < _size; ++i) {
      const auto* obj = atObject(i);
      if (obj->getName() == name)
        return i;
    }
    return _size;
  }

  SelectionState state;

  bool isSelected(std::size_t index) const {
    return std::find(state.selectedChildren.begin(),
                     state.selectedChildren.end(),
                     index) != state.selectedChildren.end();
  }
  bool select(std::size_t index) {
    if (isSelected(index))
      return true;

    state.selectedChildren.push_back(index);
    return false;
  }
  bool deselect(std::size_t index) {
    auto it = std::find(state.selectedChildren.begin(),
                        state.selectedChildren.end(), index);

    if (it == state.selectedChildren.end())
      return false;
    state.selectedChildren.erase(it);
    return true;
  }
  std::size_t clearSelection() {
    std::size_t before = state.selectedChildren.size();
    state.selectedChildren.clear();
    return before;
  }
  std::size_t getActiveSelection() const { return state.activeSelectChild; }
  std::size_t setActiveSelection(std::size_t value) {
    const std::size_t old = state.activeSelectChild;
    state.activeSelectChild = value;
    return old;
  }
};

template <typename T> struct CollectionIterator {
  CollectionIterator(ICollection* data, std::size_t i) : data(data), i(i) {}
  CollectionIterator(const CollectionIterator& src) {
    i = src.i;
    data = src.data;
  }
  ~CollectionIterator() = default;
  CollectionIterator& operator=(const CollectionIterator& rhs) {
    i = rhs.i;
    data = rhs.data;
    return *this;
  }
  CollectionIterator& operator++() { // pre
    ++i;
    return *this;
  }
  CollectionIterator operator++(int) { // post
    CollectionIterator tmp(*this);
    tmp.i++;
    return tmp;
  }
  bool operator==(const CollectionIterator& rhs) const {
    return data == rhs.data && i == rhs.i;
  }
  bool operator!=(const CollectionIterator& rhs) const {
    return !(*this == rhs);
  }
  ICollection* data;
  std::size_t i;
};

template <typename T> struct MutCollectionIterator : CollectionIterator<T> {
  typedef MutCollectionIterator<T> self_type;
  typedef T value_type;
  typedef T& reference;
  typedef T* pointer;
  typedef std::forward_iterator_tag iterator_category;
  typedef std::ptrdiff_t difference_type;

  using CollectionIterator<T>::CollectionIterator;

  T& operator*() const {
    return *reinterpret_cast<T*>(this->data->at(this->i));
  }
  T* operator->() const {
    return reinterpret_cast<T*>(this->data->at(this->i));
  }
};
template <typename T> struct ConstCollectionIterator : CollectionIterator<T> {
  typedef ConstCollectionIterator<T> self_type;
  typedef T value_type;
  typedef const T& reference;
  typedef const T* pointer;
  typedef std::forward_iterator_tag iterator_category;
  typedef std::ptrdiff_t difference_type;

  using CollectionIterator<T>::CollectionIterator;

  ConstCollectionIterator(const ICollection* data, std::size_t i)
      : CollectionIterator<T>(const_cast<ICollection*>(data),
                              CollectionIterator<T>::i) {}

  const T& operator*() const {
    return *reinterpret_cast<const T*>(this->data->at(this->i));
  }
  const T* operator->() const {
    return reinterpret_cast<const T*>(this->data->at(this->i));
  }
};
struct IDocData {
  virtual ~IDocData() = default;
  virtual IDocData& operator=(const IDocData& rhs) const = 0;
  virtual bool operator==(const IDocData& rhs) const = 0;
  virtual std::unique_ptr<IDocData> clone() const = 0;
};

template <typename T> struct TDocData : public IDocData, public T {
  virtual IDocData& operator=(const IDocData& rhs) const {
    *(T*)this = *(T*)(TDocData<T>*)&rhs;
    return *(IDocData*)this;
  }
  virtual bool operator==(const IDocData& rhs) const {
    return *(T*)this == *(T*)(TDocData<T>*)&rhs;
  }
  virtual std::unique_ptr<IDocData> clone() const {
    return std::make_unique<TDocData<T>>(*this);
  }
};

// Base of all concrete collection types
struct INode : public virtual IObject // TODO: Global factories require this.
                                      // But we don't need them?
{
  virtual ~INode() = default;
  virtual std::size_t numFolders() const = 0;
  virtual const ICollection* folderAt(std::size_t index) const = 0;
  virtual ICollection* folderAt(std::size_t index) = 0;

  // The data not stored in folders
  // Note: If no folders, this is the only data allowed
  virtual IDocData* getImmediateData() = 0;
  virtual const IDocData* getImmediateData() const = 0;

  // For memento tracking
  virtual const char* idAt(std::size_t) const = 0;
  virtual std::size_t fromId(const char*) const = 0;
};

template <typename T> class ConstCollectionRange {
public:
  bool empty() const { return low == nullptr || low->size() == 0; }
  std::size_t size() const { return low != nullptr ? low->size() : 0; }
  const T* at(std::size_t i) const {
    return low == nullptr ? nullptr : reinterpret_cast<const T*>(low->at(i));
  }
  ConstCollectionRange() : low(nullptr) {}
  ConstCollectionRange(const ICollection* src) : low(src) {}

  const T& operator[](std::size_t i) const {
    assert(at(i));
    return *at(i);
  }

  ConstCollectionIterator<T> begin() const {
    return ConstCollectionIterator<T>{low, 0};
  }
  ConstCollectionIterator<T> end() const {
    return ConstCollectionIterator<T>{low, size()};
  }

  ConstCollectionRange toConst() const { return *this; }
  template <typename U> ConstCollectionRange<U> toBase() {
    static_assert((std::is_same_v<U, T> ||
                   std::is_base_of_v<U, T>)&&"U is not a base of T");
    return {low};
  }
  template <typename U> operator ConstCollectionRange<U>() {
    return toBase<U>();
  }

  const T* findByName(const std::string_view name) const {
    const auto index = low->indexOf(name);
    return index < low->size() ? at(index) : nullptr;
  }

private:
  const ICollection* low = nullptr;
};

template <typename T> class MutCollectionRange {
public:
  bool empty() const { return low == nullptr || low->size() == 0; }
  std::size_t size() const { return low != nullptr ? low->size() : 0; }
  T* at(std::size_t i) {
    return low != nullptr ? reinterpret_cast<T*>(low->at(i)) : nullptr;
  }
  const T* at(std::size_t i) const {
    return low != nullptr ? reinterpret_cast<T*>(low->at(i)) : nullptr;
  }
  T& add() {
    assert(low != nullptr);
    const auto i = low->size();
    low->add();
    return *at(i);
  }
  MutCollectionRange() : low(nullptr) {}
  MutCollectionRange(ICollection* src) : low(src) {}

  T& operator[](std::size_t i) {
    assert(at(i));
    return *at(i);
  }
  const T& operator[](std::size_t i) const {
    assert(at(i));
    return *at(i);
  }

  MutCollectionIterator<T> begin() { return {low, 0}; }
  MutCollectionIterator<T> end() { return {low, size()}; }

  ConstCollectionIterator<T> cbegin() const { return {low, 0}; }
  ConstCollectionIterator<T> cend() const { return {low, size()}; }

  ConstCollectionRange<T> toConst() const { return {low}; }

  template <typename U> operator ConstCollectionRange<U>() {
    return toConst().template toBase<U>();
  }
  template <typename U> MutCollectionRange<U> toBase() {
    static_assert((std::is_same_v<U, T> ||
                   std::is_base_of_v<U, T>)&&"U is not a base of T");
    return {*low};
  }
  template <typename U> operator MutCollectionRange<U>() { return toBase<U>(); }

  T* findByName(const std::string_view name) {
    const auto index = low->indexOf(name);
    return index < low->size() ? at(index) : nullptr;
  }
  const T* findByName(const std::string_view name) const {
    const auto index = low->indexOf(name);
    return index < low->size() ? at(index) : nullptr;
  }

private:
  ICollection* low = nullptr;
};

template <typename T>
struct CollectionItemImpl final : public virtual IObject, public T {};

template <typename T> struct CollectionImpl final : public ICollection {
  // Rationale: It's quite common to have a single bone (static pose) and a
  // single model (formats like BMD). Perhaps in the future, this should be
  // customized further.
  llvm::SmallVector<CollectionItemImpl<T>, 1> data;
  INode* parent = nullptr;

  std::size_t size() const override { return data.size(); }
  void* at(std::size_t i) override { return static_cast<T*>(&data[i]); }
  const void* at(std::size_t i) const override {
    return static_cast<const T*>(&data[i]);
  }
  IObject* atObject(std::size_t i) override { return &data[i]; }
  const IObject* atObject(std::size_t i) const override { return &data[i]; }
  void add() override {
    auto& last = data.emplace_back();
    last.collectionOf = this;
    last.childOf = parent;
  }
  void resize(std::size_t size) override {
    data.resize(size);
    for (auto& elem : data) {
      elem.collectionOf = this;
      elem.childOf = parent;
    }
  }
  CollectionImpl(INode* _parent) : parent(_parent) {}
  CollectionImpl(const CollectionImpl& rhs)
      : data(rhs.data), parent(rhs.parent) {
    for (auto& elem : data)
      elem.collectionOf = this;
  }
  CollectionImpl(CollectionImpl&& rhs)
      : data(std::move(rhs.data)), parent(rhs.parent) {
    for (auto& elem : data)
      elem.collectionOf = this;
  }
};

} // namespace kpi
