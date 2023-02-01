#include "Zip.hpp"

#include <elzip/elzip.hpp>

namespace rsl {

void ExtractZip(std::string from_file, std::string to_folder) {
  elz::extractZip(from_file, to_folder);
}

} // namespace rsl
