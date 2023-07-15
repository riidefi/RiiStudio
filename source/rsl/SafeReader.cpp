#include "SafeReader.hpp"

#include <algorithm>
#include <ranges>

namespace rsl {

// Slicing shouldn't fail, python approach. Just returns a null span.
auto SafeReader::slice(u32 ofs) -> std::span<const u8> {
  auto pos = mReader.tell();
  if (pos >= mReader.endpos()) {
    return {};
  }
  if (pos < 0) {
    return {};
  }
  return mReader.slice().subspan(ofs);
}
void SafeReader::seekSet(u32 pos) { mReader.seekSet(pos); }
auto SafeReader::tell() const -> u32 { return mReader.tell(); }

auto SafeReader::F32() -> Result<f32> { return mReader.tryRead<f32>(); }
auto SafeReader::U32() -> Result<u32> { return mReader.tryRead<u32>(); }
auto SafeReader::S32() -> Result<s32> { return mReader.tryRead<s32>(); }
auto SafeReader::U16() -> Result<u16> { return mReader.tryRead<u16>(); }
auto SafeReader::S16() -> Result<s16> { return mReader.tryRead<s16>(); }
auto SafeReader::U8() -> Result<u8> { return mReader.tryRead<u8>(); }
auto SafeReader::S8() -> Result<s8> { return mReader.tryRead<s8>(); }

auto SafeReader::F32NoAlign() -> Result<f32> {
  return mReader.tryRead<f32, Cur, true>();
}
auto SafeReader::U32NoAlign() -> Result<u32> {
  return mReader.tryRead<u32, Cur, true>();
}
auto SafeReader::S32NoAlign() -> Result<s32> {
  return mReader.tryRead<s32, Cur, true>();
}
auto SafeReader::U16NoAlign() -> Result<u16> {
  return mReader.tryRead<u16, Cur, true>();
}
auto SafeReader::S16NoAlign() -> Result<s16> {
  return mReader.tryRead<s16, Cur, true>();
}
auto SafeReader::U8NoAlign() -> Result<u8> {
  return mReader.tryRead<u8, Cur, true>();
}
auto SafeReader::S8NoAlign() -> Result<s8> {
  return mReader.tryRead<s8, Cur, true>();
}

auto SafeReader::Bool8() -> Result<bool> {
  auto u = TRY(mReader.tryRead<u8>());
  switch (u) {
  case 0:
    return false;
  case 1:
    return true;
  default:
    return std::unexpected(
        std::format("Expected bool value (0 or 1); instead saw {}", u));
  }
}

auto SafeReader::Magic(std::string_view ident) -> Result<std::string_view> {
  auto buf = TRY(mReader.tryReadBuffer<char>(ident.size()));
  if (!std::ranges::equal(buf, ident)) {
    auto buf_s = std::string(buf.begin(), buf.end());
    auto msg =
        std::format("Expected magic identifier {} at {}. Instead saw {}.",
                    ident, mReader.tell() - ident.size(), buf_s);
    mReader.warnAt(msg.c_str(), mReader.tell() - ident.size(), mReader.tell());
    return std::unexpected(msg);
  }
  return ident;
}

SafeReader::Result<std::string> SafeReader::StringOfs32(u32 relative) {
  auto ofs = TRY(mReader.tryRead<s32>());

  // semi-likely
  if (ofs == 0) {
    return std::string("");
  }

  [[unlikely]] if (relative + ofs >= mReader.endpos() ||
                   static_cast<s32>(relative) + ofs < 0) {
    return std::unexpected(
        std::format("Invalid string offset {}. Out of file bounds [0, {})",
                    ofs + relative, mReader.endpos()));
  }

  auto span = mReader.slice().subspan(relative + ofs);
  bool terminated = false;
  for (size_t i = 0; i < span.size(); ++i) {
    if (span[i] == 0) {
      span = span.subspan(0, i);
      terminated = true;
      break;
    }
  }

  // very unlikely
  [[unlikely]] if (!terminated) {
    return std::unexpected("File has been truncated. String does not contain "
                           "a final null terminator");
  }

  return std::string{reinterpret_cast<const char*>(span.data()),
                     reinterpret_cast<const char*>(span.data() + span.size())};
}

} // namespace rsl
