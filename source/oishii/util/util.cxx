// Must be included before <bit> it seems
#ifdef _MSC_VER
#include <Windows.h>
#endif

#include <fstream>
#include "util.hxx"

namespace oishii {

Console Console::sInstance;

std::expected<std::vector<u8>, std::string>
UtilReadFile(std::string_view path) {
  std::ifstream file(std::string(path), std::ios::binary | std::ios::ate);
  if (!file) {
    return std::unexpected("Failed to open file " + std::string(path));
  }

  std::vector<u8> vec(file.tellg());
  file.seekg(0, std::ios::beg);

  if (!file.read(reinterpret_cast<char*>(vec.data()), vec.size())) {
    return std::unexpected("Failed to read file " + std::string(path));
  }
  return vec;
}

void OishiiDefaultFlushFile(std::span<const uint8_t> buf,
                            std::string_view path) {
  std::ofstream stream(std::string(path), std::ios::binary | std::ios::out);
  stream.write(reinterpret_cast<const char*>(buf.data()), buf.size());
}
FlushFileHandler s_flushFileHandler = OishiiDefaultFlushFile;

void SetGlobalFileWriteFunction(FlushFileHandler handler) {
  s_flushFileHandler = handler;
}

void FlushFile(std::span<const uint8_t> buf, std::string_view path) {
  assert(s_flushFileHandler != nullptr);
  s_flushFileHandler(buf, path);
}

} // namespace oishii
