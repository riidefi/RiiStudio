#include "WriteFile.hpp"
#include <fstream>

namespace rsl {

Result<void> WriteFile(const std::span<uint8_t> data,
                       const std::string_view path) {
  std::ofstream stream(std::string(path), std::ios::binary | std::ios::out);
  stream.write(reinterpret_cast<const char*>(data.data()), data.size());
  return {};
}

} // namespace rsl
