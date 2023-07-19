#ifdef _WIN32
#include <Windows.h>
///////////////////////////
#include <Libloaderapi.h>
#elif __APPLE__
#include <limits.h>
#include <mach-o/dyld.h>
#else
#include <unistd.h>
#endif

#include "Launch.hpp"

#include <array>

namespace rsl {

void LaunchAsAdmin(std::string path, std::string arg) {
#ifdef _WIN32
  ShellExecute(nullptr, "runas", path.c_str(), arg.c_str(), 0, SW_SHOWNORMAL);
#else
// FIXME: Provide Linux/Mac version
#endif
}
void LaunchAsUser(std::string path) {
#ifdef _WIN32
  // https://docs.microsoft.com/en-us/windows/win32/procthread/creating-processes

  STARTUPINFO si;
  PROCESS_INFORMATION pi;

  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  ZeroMemory(&pi, sizeof(pi));

  std::array<char, 32> buf{};

  CreateProcessA(path.c_str(), // lpApplicationName
                 buf.data(),   // lpCommandLine
                 nullptr,      // lpProcessAttributes
                 nullptr,      // lpThreadAttributes
                 false,        // bInheritHandles
                 0,            // dwCreationFlags
                 nullptr,      // lpEnvironment
                 nullptr,      // lpCurrentDirectory
                 &si,          // lpStartupInfo
                 &pi           // lpProcessInformation
  );

  // TODO: If we were launched as an admin, drop those permissions
  // AdjustTokenPrivileges(nullptr, true, nullptr, 0, nullptr, 0);

#else
  // FIXME: Provide Linux/Mac version
#endif
}

std::string GetExecutableFilename() {
#ifdef _WIN32
  std::array<char, 1024> pathBuffer{};
  const int n =
      GetModuleFileNameA(nullptr, pathBuffer.data(), pathBuffer.size());

  // We don't want a truncated path
  if (n < 1020)
    return std::string(pathBuffer.data(), n);

#elif __APPLE__
  std::array<char, 1024> pathBuffer{};
  uint32_t size = pathBuffer.size();

  if (_NSGetExecutablePath(pathBuffer.data(), &size) == 0)
    return std::string(pathBuffer.data());
  else
    return ""; // buffer too small

#else // Assume Linux
  std::array<char, 1024> pathBuffer{};
  ssize_t len =
      readlink("/proc/self/exe", pathBuffer.data(), pathBuffer.size() - 1);

  if (len != -1) {
    pathBuffer[len] = '\0';
    return std::string(pathBuffer.data());
  }
#endif
  return "";
}
} // namespace rsl
