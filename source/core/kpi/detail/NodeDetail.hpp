#pragma once

#include <core/kpi/Node2.hpp>
#include <core/kpi/Reflection.hpp>

namespace kpi {

namespace detail {

template <typename Q> class has_render {
  typedef char YesType[1];
  typedef char NoType[2];

  template <typename C> static YesType& test(decltype(&C::render));
  template <typename C> static NoType& test(...);

public:
  enum { value = sizeof(test<Q>(0)) == sizeof(YesType) };
};

template <typename Q> class has_add_bp {
  typedef char YesType[1];
  typedef char NoType[2];

  template <typename C> static YesType& test(decltype(&C::addBp));
  template <typename C> static NoType& test(...);

public:
  enum { value = sizeof(test<Q>(0)) == sizeof(YesType) };
};

struct ApplicationPluginsImpl {
  // Requires TypeIdResolvable<T>, DefaultConstructible<T>
  template <typename T>
  struct TFactory final : public ApplicationPlugins::IFactory {
    std::unique_ptr<IFactory> clone() const override {
      return std::make_unique<TFactory<T>>(*this);
    }
    std::unique_ptr<IObject> spawn() override {
      return std::unique_ptr<IObject>{static_cast<IObject*>(new T())};
    }
    const char* getId() const override { return typeid(T).name(); }
  };
  //! Requires methods:
  //! - `T::canRead(IOTransaction&) const
  //! const`
  //! - `T::read(doc_node_t node, oishii::ByteView data)`
  template <typename T>
  struct TBinaryDeserializer final : public IBinaryDeserializer, public T {
    std::unique_ptr<IBinaryDeserializer> clone() const override {
      return std::make_unique<TBinaryDeserializer<T>>(*this);
    }
    std::string canRead_(const std::string& file,
                         oishii::BinaryReader& reader) const override {
      return T::canRead(file, reader);
    }
    void read_(IOTransaction& transaction) override {
#if !defined(HAS_RUST_TRY)
      try {
#endif // !defined(HAS_RUST_TRY)
        T::read(transaction);
#if !defined(HAS_RUST_TRY)
      } catch (std::string s) {
        transaction.callback(IOMessageClass::Error, "MSVC hack", s);
        transaction.state = TransactionState::Failure;
      }
#endif // !defined(HAS_RUST_TRY)
    }
    void render() override {
      if constexpr (has_render<T>::value)
        T::render();
    }

    void addBp(u32 addr) override {
      if constexpr (has_add_bp<T>::value)
        T::addBp(addr);
    }
  };
  //! Requires methods:
  //! - `T::canWrite(doc_node_t node) const`
  //! - `T::write(kpi::INode& node, oishii::Writer& writer) const`
  template <typename T>
  struct TBinarySerializer final : public IBinarySerializer, public T {
    std::unique_ptr<IBinarySerializer> clone() const override {
      return std::make_unique<TBinarySerializer<T>>(*this);
    }
    bool canWrite_(kpi::INode& node) const override {
      return T::canWrite(node);
    }
    void write_(kpi::INode& node, oishii::Writer& writer) const override {
      T::write(node, writer);
    }
  };
  //! Requires: `::write(doc_node_t, oishii::Writer& writer, X*_=nullptr)`
  //! where `X` is some child that may be wrapped in a doc_node_t. No support
  //! for inheritance.
  template <typename T>
  struct TSimpleBinarySerializer final : public IBinarySerializer {
    std::unique_ptr<IBinarySerializer> clone() const override {
      return std::make_unique<TSimpleBinarySerializer<T>>(*this);
    }
    bool canWrite_(kpi::INode& node) const override {
      return dynamic_cast<T*>(&node) != nullptr;
    }
    void write_(kpi::INode& node, oishii::Writer& writer) const override {
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

} // namespace kpi
