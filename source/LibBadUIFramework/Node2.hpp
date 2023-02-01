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
  virtual void setName(const std::string& name) { (void)name; }

  // For now, at least, all objects exist in collections
  ICollection* collectionOf = nullptr;
  // The owner of the collection
  INode* childOf = nullptr;
};

struct INamed {
  virtual std::string getName() const = 0;
  virtual void setName(const std::string& name) = 0;
};

template <typename T> std::unique_ptr<INamed> DynNamed(const T& data) {
  struct Impl : public INamed {
    std::string getName() const override { return m_data->getName(); }
    void setName(const std::string& name) { m_data->setName(name); }
    Impl(T& data) : m_data(&data) {}
    T* m_data;
  };
  return std::make_unique<Impl>(data);
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
  virtual std::string atName(std::size_t) const = 0;
  virtual void add() = 0;

  virtual void swap(std::size_t, std::size_t) = 0;

  std::size_t indexOf(const std::string_view name) const {
    const auto _size = size();
    for (std::size_t i = 0; i < _size; ++i) {
      if (atName(i) == name) {
        return i;
      }
    }
    return _size;
  }
};

template <typename T, typename Derived> struct CollectionIterator {
  CollectionIterator(ICollection* data_, std::size_t i_) : data(data_), i(i_) {}
  CollectionIterator(const CollectionIterator& src) {
    i = src.i;
    data = src.data;
  }
  CollectionIterator() : data(nullptr), i(0) {}
  ~CollectionIterator() = default;
  CollectionIterator& operator=(const CollectionIterator& rhs) {
    i = rhs.i;
    data = rhs.data;
    return *this;
  }
  Derived& operator++() { // pre
    ++i;
    return static_cast<Derived&>(*this);
  }
  Derived operator++(int) { // post
    Derived tmp(static_cast<Derived&>(*this));
    ++tmp.i;
    return tmp;
  }
  std::ptrdiff_t operator-(const CollectionIterator& rhs) const {
    return i - rhs.i;
  }
  bool operator==(const CollectionIterator& rhs) const = default;
  bool operator!=(const CollectionIterator& rhs) const {
    return !(*this == rhs);
  }
  ICollection* data = nullptr;
  std::size_t i = 0;
};

template <typename T>
struct MutCollectionIterator : CollectionIterator<T, MutCollectionIterator<T>> {
  typedef MutCollectionIterator<T> self_type;
  typedef T value_type;
  typedef T& reference;
  typedef T* pointer;
  typedef std::forward_iterator_tag iterator_category;
  typedef std::ptrdiff_t difference_type;

  using CollectionIterator<T, MutCollectionIterator<T>>::CollectionIterator;

  T& operator*() const {
    return *reinterpret_cast<T*>(this->data->at(this->i));
  }
  T* operator->() const {
    return reinterpret_cast<T*>(this->data->at(this->i));
  }
};
template <typename T>
struct ConstCollectionIterator
    : CollectionIterator<T, ConstCollectionIterator<T>> {
  typedef ConstCollectionIterator<T> self_type;
  typedef T value_type;
  typedef const T& reference;
  typedef const T* pointer;
  typedef std::forward_iterator_tag iterator_category;
  typedef std::ptrdiff_t difference_type;

  using CollectionIterator<T, ConstCollectionIterator<T>>::CollectionIterator;

  ConstCollectionIterator(const ICollection* data_, std::size_t i_)
      : CollectionIterator<T, ConstCollectionIterator<T>>(
            const_cast<ICollection*>(data_), i_) {}

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
    *(T*)this = *(const T*)(const TDocData<T>*)&rhs;
    return *(IDocData*)this;
  }
  virtual bool operator==(const IDocData& rhs) const {
    return *(T*)this == *(const T*)(const TDocData<T>*)&rhs;
  }
  virtual std::unique_ptr<IDocData> clone() const {
    return std::make_unique<TDocData<T>>(*this);
  }
};
struct IMemento;

struct ITypeFolderManager {
  virtual ~ITypeFolderManager() = default;

  virtual std::size_t numFolders() const = 0;
  virtual const ICollection* folderAt(std::size_t index) const = 0;
  virtual ICollection* folderAt(std::size_t index) = 0;

  // The data not stored in folders
  // Note: If no folders, this is the only data allowed
  virtual IDocData* getImmediateData() = 0;
  virtual const IDocData* getImmediateData() const = 0;

  virtual const char* idAt(std::size_t) const = 0;
  virtual std::size_t fromId(const char*) const = 0;
};

struct IMementoOriginator {
  virtual ~IMementoOriginator() = default;

  virtual std::unique_ptr<kpi::IMemento>
  next(const kpi::IMemento* last) const = 0;
  virtual void from(const kpi::IMemento& memento) = 0;
};

// Base of all concrete collection types
struct INode : public ITypeFolderManager,
               public IMementoOriginator,
               public virtual IObject // TODO: Global factories require this.
                                      // But we don't need them?
{
  virtual ~INode() override = default;

  // virtual std::unique_ptr<INode> clone() const = 0;
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
  ConstCollectionRange(const ConstCollectionRange& src) : low(src.low) {
    assert(src.low);
    assert(src.low->size() < 0xFFFF);
  }

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

  s32 indexOf(const std::string_view name) const {
    if (name.empty())
      return -1;
    const auto low_index = low->indexOf(name);
    // Unmanaged indexOf returns the size of the collection when not found (like
    // an iterator)
    return low_index != low->size() ? low_index : -1;
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
  void resize(std::size_t sz) { low->resize(sz); }
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

  MutCollectionRange<T>& operator=(const MutCollectionRange<T>& rhs) {
    resize(rhs.size());
    for (int i = 0; i < rhs.size(); ++i)
      operator[](i) = rhs[i];
    return *this;
  }

private:
  ICollection* low = nullptr;
};

template <typename T>
struct CollectionItemImpl final : public virtual IObject, public T {
  CollectionItemImpl(const CollectionItemImpl& rhs) : T(rhs) {
    collectionOf = rhs.collectionOf;
    childOf = rhs.childOf;
  }
  CollectionItemImpl(CollectionItemImpl&& rhs) noexcept : T(std::move(rhs)) {
    collectionOf = rhs.collectionOf;
    childOf = rhs.childOf;
  }
  CollectionItemImpl() {
    collectionOf = nullptr;
    childOf = nullptr;
  }
};

template <typename T> struct CollectionImpl final : public ICollection {
  using element_type = CollectionItemImpl<T>;
  using boxed_type = std::unique_ptr<element_type>;

  // Rationale: It's quite common to have a single bone (static pose) and a
  // single model (formats like BMD). Perhaps in the future, this should be
  // customized further.
#ifndef BUILD_DEBUG
  llvm::SmallVector<boxed_type, 1> data;
#else
  std::vector<boxed_type> data;
#endif
  INode* parent = nullptr;

  std::size_t size() const override { return data.size(); }
  void* at(std::size_t i) override {
    assert(i < data.size());
    return static_cast<T*>(data[i].get());
  }
  const void* at(std::size_t i) const override {
    assert(i < data.size());
    return static_cast<const T*>(data[i].get());
  }
  IObject* atObject(std::size_t i) override { return data[i].get(); }
  const IObject* atObject(std::size_t i) const override {
    return data[i].get();
  }
  std::string atName(std::size_t i) const override {
    assert(i < data.size());
    return static_cast<T&>(*data[i]).getName();
  }
  void add() override {
    auto& last = data.emplace_back(std::make_unique<element_type>());
    last->collectionOf = this;
    last->childOf = parent;
  }
  void resize(std::size_t size) override {
    auto old_size = data.size();
    data.resize(size);

    if (size > old_size) {
      for (size_t i = old_size; i < size; ++i) {
        data[i] = std::make_unique<element_type>();
      }
    }

    for (auto& elem : data) {
      elem->collectionOf = this;
      elem->childOf = parent;
    }
  }
  void swap(std::size_t a, std::size_t b) override {
    std::swap(data[a], data[b]);
  }
  CollectionImpl(INode* _parent) : parent(_parent) {}
  CollectionImpl(const CollectionImpl& rhs) : parent(rhs.parent) {
    data.resize(rhs.data.size());
    size_t i = 0;
    for (auto& elem : data) {
      elem = std::make_unique<element_type>(*rhs.data[i++]);

      elem->collectionOf = this;
      elem->childOf = parent;
    }
  }
  CollectionImpl(CollectionImpl&& rhs)
      : data(std::move(rhs.data)), parent(rhs.parent) {
    for (auto& elem : data) {
      elem->collectionOf = this;
      elem->childOf = parent;
    }
  }

  void onParentMoved(INode* _parent) {
    parent = _parent;
    for (auto& elem : data) {
      elem->collectionOf = this;
      elem->childOf = _parent;
    }
  }
};

// Memento

struct IMemento {
  virtual ~IMemento() = default;
};
template <typename T, typename V = void> struct _MementoIfy {
  using _type = T;
};
template <typename T> struct _MementoIfy<T, std::void_t<typename T::_Memento>> {
  using _type = typename T::_Memento;
};
template <typename T> using MementoIfy = typename _MementoIfy<T>::_type;

template <typename T>
using ConstPersistentVec = std::vector<std::shared_ptr<const MementoIfy<T>>>;

template <typename T, typename U> bool should_set(const T* out, const U* in) {
  assert(in);
  if (out == nullptr)
    return true;

  if constexpr (std::is_same_v<std::remove_const<T>, std::remove_const<U>>) {
    if (*out == *in)
      return false;
  }

  return true;
}

template <typename R, typename T, typename U>
std::shared_ptr<R> set_m(const T* last, const U& in) {
  if constexpr (!std::is_same_v<std::remove_const<T>, std::remove_const<U>>) {
    return std::make_shared<T>(in, last);
  } else {
    return std::make_shared<T>(in);
  }
}

// Create a composite memento
template <typename InT, typename OutT, typename OldT>
void nextFolder(OutT& out, const InT& in, const OldT* old) {
  using record_t = MementoIfy<typename OutT::value_type::element_type>;
  if (old != nullptr) {
    auto& last = *old;
    out.resize(in.size());
    for (size_t i = 0; i < in.size(); ++i) {
      if (i < last.size() && should_set(last[i].get(), &in[i])) {
        out[i] = set_m<record_t>(last[i].get(), in[i]);
      } else {
        out[i] = std::make_shared<const record_t>(in[i]);
      }
    }
  } else {
    out.resize(in.size());
    for (size_t i = 0; i < in.size(); ++i) {
      out[i] = std::make_shared<const record_t>(in[i]);
    }
  }
}

template <typename Q> class has_notify_observers {
  typedef char YesType[1];
  typedef char NoType[2];

  template <typename C> static YesType& test(decltype(&C::notifyObservers));
  template <typename C> static NoType& test(...);

public:
  enum { value = sizeof(test<Q>(0)) == sizeof(YesType) };
};

template <typename CType, typename MType>
void set_concrete_element(CType& out, const MType& memento) {
  out = memento;
  if constexpr (has_notify_observers<CType>::value)
    out.notifyObservers();
}

// Restore a concrete object from a memento
template <typename InT, typename OutT>
void fromFolder(OutT&& out /*rvalue range*/, const InT& in) {
  const auto both = std::min(in.size(), out.size());
  for (size_t i = 0; i < both; ++i) {
    if (should_set(&out[i], in[i].get())) {
      set_concrete_element(out[i], *in[i].get());
    }
  }
  if (in.size() < out.size()) {
    out.resize(in.size());
  } else if (in.size() > out.size()) {
    const auto added = in.size() - out.size();
    out.resize(in.size());
    for (int i = in.size() - added; i < in.size(); ++i) {
      out[i] = *in[i];
      // Observers are not notified here.
      // Rationale: New objects likely do not have observers.
    }
  }
}
} // namespace kpi
