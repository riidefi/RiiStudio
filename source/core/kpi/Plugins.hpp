#pragma once

#include <memory>
#include <string>
#include <map>
#include <vector>
#include "Reflection.hpp"

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
  //!					(return if we can write)
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
