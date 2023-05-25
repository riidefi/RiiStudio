#include "Zip.hpp"

#include <cstdint>
#include <fstream>
#include <span>

extern "C" int c_rsl_extract_zip(const char* from_file, const char* to_folder);

namespace rsl {

// This is now part of a dependency of riidefi/zip-extract
#ifdef RSL_V1
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
#endif

void ExtractZip(std::string from_file, std::string to_folder) {
  (void)c_rsl_extract_zip(from_file.c_str(), to_folder.c_str());
}

} // namespace rsl
