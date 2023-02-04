#include "Zip.hpp"

#define DONT_USE_RUST

#ifdef DONT_USE_RUST
#include <elzip/elzip.hpp>
#else
extern "C" int c_rsl_extract_zip(const char* from_file, const char* to_folder);
#endif

namespace rsl {

void ExtractZip(std::string from_file, std::string to_folder) {
#ifdef DONT_USE_RUST
  elz::extractZip(from_file, to_folder);
#else
  (void)c_rsl_extract_zip(from_file.c_str(), to_folder.c_str());
#endif
}

} // namespace rsl
