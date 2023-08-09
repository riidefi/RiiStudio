#ifdef _WIN32
#include <windows.h>

#include <Psapi.h>
#include <tlhelp32.h>
#endif

#include <core/common.h>
#include <optional>

#include <iomanip>
#include <iostream>

#include "Dolphin.hpp"

namespace librii::dolphin {

std::optional<int> GetDolphinPID() {
#ifdef _WIN32
  PROCESSENTRY32 entry;
  entry.dwSize = sizeof(PROCESSENTRY32);

  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

  if (Process32First(snapshot, &entry) == TRUE) {
    do {
      if (std::string(entry.szExeFile) == "Dolphin.exe" ||
          std::string(entry.szExeFile) == "DolphinQt2.exe" ||
          std::string(entry.szExeFile) == "DolphinWx.exe") {
        return entry.th32ProcessID;
      }
    } while (Process32Next(snapshot, &entry) == TRUE);
  }

  CloseHandle(snapshot);
#endif
  return std::nullopt;
}

SharedMem::~SharedMem() {
  if (sharedMem != nullptr) {
#ifdef _WIN32
    UnmapViewOfFile(sharedMem);
#endif
    sharedMem = nullptr;
  }
}
Result<SharedMem> SharedMem::from(std::string memFileName) {
#ifdef _WIN32
  HANDLE hMapFile =
      CreateFileMappingA(INVALID_HANDLE_VALUE, // Use paging file.
                         NULL,                 // Default security.
                         PAGE_READWRITE,       // Read/write access.
                         0,       // Maximum object size (high-order DWORD).
                         1 << 30, // Maximum object size (low-order DWORD).
                         memFileName.c_str() // Name of mapping object.
      );

  if (hMapFile == NULL) {
    return std::unexpected("Could not create file mapping object: " +
                           std::to_string(GetLastError()));
  }

  // Map the file to the process's address space.
  LPVOID lpMapAddress =
      MapViewOfFile(hMapFile,            // Handle to mapping object.
                    FILE_MAP_ALL_ACCESS, // Read/write permission.
                    0, 0,
                    0 // Map the entire file.
      );

  if (lpMapAddress == NULL) {
    auto s = "Could not map view of file: " + std::to_string(GetLastError());
    CloseHandle(hMapFile);
    return std::unexpected(s);
  }

  return SharedMem(lpMapAddress);
#else
  return std::unexpected("Non-windows unsuported");
#endif
}

Result<SharedMem> OpenDolphin(int pid) {
  std::string memFileName = "dolphin-emu." + std::to_string(pid);
  return SharedMem::from(memFileName);
}

void DolphinAc::dumpMemoryLayout() {
  std::cout << "Memory Layout Dump:" << std::endl;
  std::cout << "------------------------------------------------------"
            << std::endl;
  std::cout << "| Region    | Start Virtual | End Virtual | Size (MB) |"
            << std::endl;
  std::cout << "------------------------------------------------------"
            << std::endl;

  dumpRegion("RAM", 0x80000000, GetRamSize(mSHM));
  dumpRegion("L1 Cache", 0xE0000000, GetL1CacheSize());
  dumpRegion("Fake VMEM", 0x7E000000, GetFakeVMemSize());
  dumpRegion("ExRAM", 0x90000000, GetExRamSize());

  std::cout << "------------------------------------------------------"
            << std::endl;
}

void DolphinAc::dumpRegion(const std::string& name, u32 virtualStart,
                           u32 size) {
  std::cout << "| " << std::left << std::setw(10) << name << "| 0x"
            << std::setw(13) << std::hex << virtualStart << "| 0x"
            << std::setw(11) << std::hex << (virtualStart + size) << "| "
            << std::setw(8) << std::dec << (size / (1024 * 1024)) << "| "
            << std::endl;
}

} // namespace librii::dolphin
