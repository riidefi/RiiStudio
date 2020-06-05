#pragma once

#include "Reflection.hpp"
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace oishii {
class BinaryReader;
namespace v2 {
class Writer;
}
} // namespace oishii

namespace kpi {

class IDocumentNode;

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
  //! @brief	Add a type to the internal registry for future construction and
  //!			manipulation.
  //!
  //! @tparam T Any default constructible type.
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
  //!					(return if we can write)
  //!				- `T::write(doc_node_t node, oishii::v2::Writer&
  //! writer) const` (write the file)
  //!
  template <typename T> ApplicationPlugins& addSerializer();

  //! @brief Add a binary serializer (writer) to the internal registry with a
  //! simplified API.
  //!
  //! @tparam T Any default constructible type T where the following function
  //! exists:
  //! - `::write(doc_node_t, oishii::v2::Writer&
  //!			 writer, X*_=nullptr)`
  //!   where `X` is some child that may be wrapped in a doc_node_t.
  //!
  template <typename T> ApplicationPlugins& addSimpleSerializer();

  //! @brief Add a binary deserializer (reader) to the internal registry.
  //!
  //! @tparam T Any default constructible type T with member functions:
  //! - `std::string // return the type of the file identified, or empty
  //!	T::canRead(
  //!			   const std::string& file,
  //!			   oishii::BinaryReader& reader) const`
  //! - `T::read(doc_node_t node,
  //!			 oishii::BinaryReader& reader) const`
  //!
  template <typename T> ApplicationPlugins& addDeserializer();

  virtual void registerMirror(const kpi::MirrorEntry& entry);

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

  virtual void installModule(const std::string& path);

  virtual std::unique_ptr<kpi::IDocumentNode>
  constructObject(const std::string& type,
                  kpi::IDocumentNode* parent = nullptr) const;

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

private:
  std::unique_ptr<kpi::IDocumentNode> spawnState(const std::string& type) const;
};

/** Decentralized initialization via global static initializers.
 * The caveat here is that we can't guarantee any order here.
 * As we cannot
 * force ApplicationPlugins to be initialized first, we simply construct a
 * singly linked list and defer registration.
 */
class RegistrationLink {
public:
  virtual ~RegistrationLink() = default;

  RegistrationLink() {
    last = gRegistrationChain;
    gRegistrationChain = this;
  }
  RegistrationLink(RegistrationLink* last) : last(last) {}
  virtual void exec(ApplicationPlugins& registrar) {}

  static RegistrationLink* getHead() { return gRegistrationChain; }
  RegistrationLink* getLast() { return last; }

private:
  RegistrationLink* last;
  static RegistrationLink* gRegistrationChain;
};

template <typename T> class DecentralizedInstaller : private RegistrationLink {
public:
  DecentralizedInstaller(T functor) : mFunctor(functor) {}
  void exec(ApplicationPlugins& registrar) override { mFunctor(registrar); }

private:
  T mFunctor;
};

enum RegisterFlags {
  Serializer = (1 << 0),
  Deserializer = (1 << 1),
  SimpleSerializer = (1 << 2),

  // More standard aliases
  Writer = Serializer,
  Reader = Deserializer,
  SimpleWriter = SimpleSerializer
};

template <typename T, int flags> class Register : private RegistrationLink {
  void exec(ApplicationPlugins& registrar) override {
    if constexpr ((flags & Serializer) != 0) {
      registrar.addSerializer<T>();
    } else if constexpr ((flags & SimpleSerializer) != 0) {
      registrar.addSimpleSerializer<T>();
    }
    if constexpr ((flags & Deserializer) != 0) {
      registrar.addDeserializer<T>();
    }
  }
};

} // namespace kpi
