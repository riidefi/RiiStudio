#include "Zip.hpp"

#include <fstream>
#include <span>

#ifdef DONT_USE_RUST
#include <elzip/elzip.hpp>
#else
extern "C" int c_rsl_extract_zip(const char* from_file, const char* to_folder,
                                 void (*write_file)(const char* path,
                                                    const void* buf,
                                                    uint32_t len));
#endif

namespace rsl {

//
// I don't know why this works, but it does.
// In particular, this is necessary for .dll files open by the main app.
// Rust's standard library will refuse to write to them.
//
static void WriteToOpenFile(std::string_view path,
                            std::span<const char> dumped) {
  std::ofstream wFile;
  wFile.open(std::string(path), std::ios_base::binary | std::ios_base::out);
  wFile.write(dumped.data(), dumped.size());
  wFile.close();
}

void ExtractZip(std::string from_file, std::string to_folder) {
#ifdef DONT_USE_RUST
  elz::extractZip(from_file, to_folder);
#else
  (void)c_rsl_extract_zip(
      from_file.c_str(), to_folder.c_str(),
      +[](const char* path, const void* buf, uint32_t len) {
        WriteToOpenFile(path, {reinterpret_cast<const char*>(buf),
                               static_cast<size_t>(len)});
      });
#endif
}

} // namespace rsl
