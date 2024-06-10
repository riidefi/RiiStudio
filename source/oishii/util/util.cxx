// Must be included before <bit> it seems
#ifdef _MSC_VER
#include <Windows.h>
#endif

#include "util.hxx"
#include <fstream>

// TODO: Bad dependency
#include <rsl/WriteFile.hpp>

namespace oishii {

Console Console::sInstance;

#ifdef _MSC_VER
Console::Console() : handle(GetStdHandle(STD_OUTPUT_HANDLE)) {
  prior_state = std::make_unique<CONSOLE_SCREEN_BUFFER_INFO>();
  GetConsoleScreenBufferInfo((HANDLE)handle, prior_state.get());
  last = prior_state->wAttributes;
}

Console::~Console() { restoreFirstColorState(); }

void Console::restoreFirstColorState() {
  SetConsoleTextAttribute((HANDLE)handle, prior_state->wAttributes);
}

void Console::setColorState(uint16_t attrib) {
  last = attrib;
  SetConsoleTextAttribute((HANDLE)handle, attrib);
}

Console& Console::getInstance() { return sInstance; }

ScopedFormatter::ScopedFormatter(uint16_t attrib) {
  last = Console::getInstance().last;
  Console::getInstance().setColorState(attrib);
}

ScopedFormatter::~ScopedFormatter() {
  Console::getInstance().setColorState(last);
}
#else
void Console::restoreFirstColorState() {
  // Empty implementation for non-Windows platforms
}

void Console::setColorState(uint16_t attrib) {
  // Empty implementation for non-Windows platforms
}

Console& Console::getInstance() { return sInstance; }
#endif

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
  auto ok = rsl::WriteFile(buf, path);
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
