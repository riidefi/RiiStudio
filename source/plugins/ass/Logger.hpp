#pragma once

#include <core/kpi/Plugins.hpp>
#include <functional>
#include <string>
#include <string_view>
#include <vendor/assimp/Logger.hpp>

namespace riistudio::ass {

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
#ifdef BUILD_DEBUG
    // mCallback(kpi::IOMessageClass::Information, domain, message);
#endif
  }
  void OnInfo(const char* message) override {
    // mCallback(kpi::IOMessageClass::Information, domain, message);
  }
  void OnWarn(const char* message) override {
    // mCallback(kpi::IOMessageClass::Warning, domain, message);
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

} // namespace riistudio::ass
