#pragma once

#include "Node2.hpp"

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

struct LightIOTransaction {
  // Deserializer -> Caller
  std::function<void(IOMessageClass message_class,
                     const std::string_view domain,
                     const std::string_view message_body)>
      callback;
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

  template <typename... T>
  void require(bool cond, std::string_view f_, T&&... args) {
    // TODO: Use basic_format_string
    auto msg = std::vformat(f_, std::make_format_args(args...));
    require(cond, msg);
  }

  IOContext(std::string&& p, kpi::LightIOTransaction& t)
      : path(std::move(p)), transaction(t) {}
  IOContext(kpi::IOContext& c) : path(c.path), transaction(c.transaction) {}
  IOContext(kpi::LightIOTransaction& t) : transaction(t) {}
};

} // namespace kpi
