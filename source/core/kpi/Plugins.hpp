#pragma once

#include "Node2.hpp"
#include "Reflection.hpp"
#include <format>
#include <functional>
#include <map>
#include <memory>
#include <oishii/data_provider.hxx>
#include <string>
#include <vector>

namespace oishii {
class BinaryReader;
class Writer;
} // namespace oishii

namespace kpi {

enum class IOMessageClass : u32 {
  None = 0,
  Information = (1 << 0),
  Warning = (1 << 1),
  Error = (1 << 2)
};

inline IOMessageClass operator|(const IOMessageClass lhs,
                                const IOMessageClass rhs) {
  return static_cast<IOMessageClass>(static_cast<u32>(lhs) |
                                     static_cast<u32>(rhs));
}

//! @brief A (de)serializer may request input from the user.
//!
//! Configure? -> Interrupted?* (not available for clis) -> Complete
enum class TransactionState {
  Complete,            //!< Operation has ended
  Failure,             //!< Exit
  FailureToSave,       //!< Exit
  ConfigureProperties, //!< Configure + Interrupted
  ResolveDependencies, //!< Interrupted: The entire point of this system!
};

struct LightIOTransaction {
  // Deserializer -> Caller
  std::function<void(IOMessageClass message_class,
                     const std::string_view domain,
                     const std::string_view message_body)>
      callback;

  TransactionState state = TransactionState::Complete;
};

struct IOContext {
  std::string path;
  kpi::LightIOTransaction& transaction;

  auto sublet(const std::string& dir) {
    return IOContext(path + "/" + dir, transaction);
  }

  void callback(kpi::IOMessageClass mclass, std::string_view message) {
    transaction.callback(mclass, path, message);
  }

  void inform(std::string_view message) {
    callback(kpi::IOMessageClass::Information, message);
  }
  void warn(std::string_view message) {
    callback(kpi::IOMessageClass::Warning, message);
  }
  void error(std::string_view message) {
    callback(kpi::IOMessageClass::Error, message);
  }
  void request(bool cond, std::string_view message) {
    if (!cond)
      warn(message);
  }
  void require(bool cond, std::string_view message) {
    if (!cond)
      error(message);
  }

  template <typename... T>
  void request(bool cond, std::string_view f_, T&&... args) {
    // TODO: Use basic_format_string
    auto msg = std::vformat(f_, std::make_format_args(args...));
    request(cond, msg);
  }

  IOContext(std::string&& p, kpi::LightIOTransaction& t)
      : path(std::move(p)), transaction(t) {}
  IOContext(kpi::LightIOTransaction& t) : transaction(t) {}
};

struct IOTransaction : public LightIOTransaction {
  // Caller -> Deserializer
  kpi::INode& node;
  oishii::ByteView data;

  // Unresolved
  llvm::SmallVector<std::string, 8> unresolvedFiles; // request
  llvm::SmallVector<std::vector<u8>, 8>
      resolvedFiles; // reply: size 0 -> ignore
};

//! A reader: Do not inherit from this type directly
struct IBinaryDeserializer {
  virtual ~IBinaryDeserializer() = default;
  virtual std::unique_ptr<IBinaryDeserializer> clone() const = 0;
  virtual std::string canRead_(const std::string& file,
                               oishii::BinaryReader& reader) const = 0;
  virtual void read_(IOTransaction& transaction) = 0;
  //! For config UIs. Given ImGui control.
  virtual void render() = 0;
};
//! A writer: Do not inherit from this type directly
struct IBinarySerializer {
  virtual ~IBinarySerializer() = default;
  virtual std::unique_ptr<IBinarySerializer> clone() const = 0;
  virtual bool canWrite_(kpi::INode& node) const = 0;
  virtual void write_(kpi::INode& node, oishii::Writer& writer) const = 0;
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
  //!				- `T::write(doc_node_t node, oishii::Writer&
  //! writer) const` (write the file)
  //!
  template <typename T> ApplicationPlugins& addSerializer();

  //! @brief Add a binary serializer (writer) to the internal registry with a
  //! simplified API.
  //!
  //! @tparam T Any default constructible type T where the following function
  //! exists:
  //! - `::write(doc_node_t, oishii::Writer&
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
  //! - `T::read(IOTransaction&) const`
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

  virtual std::unique_ptr<kpi::IObject>
  constructObject(const std::string& type, kpi::INode* parent = nullptr) const;

  static inline ApplicationPlugins* getInstance() { return &sInstance; }
  virtual ~ApplicationPlugins() = default;

public:
  static ApplicationPlugins sInstance;

  struct IFactory {
    virtual ~IFactory() = default;
    virtual std::unique_ptr<IFactory> clone() const = 0;
    virtual std::unique_ptr<IObject> spawn() = 0;
    virtual const char* getId() const = 0;
  };

  std::map<std::string, std::unique_ptr<IFactory>> mFactories;
  std::vector<std::unique_ptr<IBinaryDeserializer>> mReaders;
  std::vector<std::unique_ptr<IBinarySerializer>> mWriters;

private:
  std::unique_ptr<kpi::IObject> spawnState(const std::string& type) const;
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

  explicit RegistrationLink(bool link = true) {
    if (!link)
      return;
    if (gRegistrationChain != nullptr)
      gRegistrationChain->next = this;
    last = gRegistrationChain;
    gRegistrationChain = this;
  }
  explicit RegistrationLink(RegistrationLink* last)
      : next(nullptr), last(last) {}
  virtual void exec(ApplicationPlugins& registrar) {}

  static RegistrationLink* getHead() { return gRegistrationChain; }
  RegistrationLink* getLast() { return last; }

public: // todo
  RegistrationLink* next = nullptr;
  RegistrationLink* last = nullptr;
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

#include <core/kpi/detail/NodeDetail.hpp>
