#pragma once

#include <LibBadUIFramework/Plugins.hpp>
#include <functional>
#include <string>
#include <string_view>
#include <vendor/assimp/Logger.hpp>

namespace librii::assimp2rhst {

struct AssimpLogger : public Assimp::Logger {
  AssimpLogger(
      std::function<void(kpi::IOMessageClass message_class,
                         const std::string_view domain,
                         const std::string_view message_body)>& callback,
      const std::string& _domain)
      : mCallback(callback), domain(_domain) {
    m_Severity = static_cast<LogSeverity>(Debugging | Info | Warn | Err);
  }
  void OnDebug(const char* message) override {
    rsl::trace("[Assimp::Debug] {}", message);
  }
  virtual void OnVerboseDebug(const char* message) /*override*/ {
    rsl::trace("[Assimp::VerboseDebug] {}", message);
  }
  void OnInfo(const char* message) override {
    rsl::trace("[Assimp::Info] {}", message);
  }
  void OnWarn(const char* message) override {
    rsl::trace("[Assimp::Warn] {}", message);
  }
  void OnError(const char* message) override {
    mCallback(kpi::IOMessageClass::Error, domain, message);
  }

  // ???
  bool attachStream(Assimp::LogStream* pStream,
                    unsigned int severity) override {
    return false;
  }
  bool detatchStream(Assimp::LogStream* pStream,
                     unsigned int severity) override {
    return false;
  }

private:
  std::function<void(kpi::IOMessageClass message_class,
                     const std::string_view domain,
                     const std::string_view message_body)>& mCallback;
  std::string domain;
};

} // namespace librii::assimp2rhst
