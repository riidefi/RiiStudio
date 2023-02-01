#ifdef _WIN32
#include <Windows.h>
///////////////////////////
#include <Libloaderapi.h>
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

} // namespace rsl
