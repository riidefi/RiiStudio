#pragma once

#include "binary_reader.hxx"

namespace oishii {

template <u32 m> struct MagicInvalidity;

template <typename T, EndianSelect E, bool unaligned> T BinaryReader::peek() {
  boundsCheck(sizeof(T));

#ifndef NDEBUG
  for (const auto& bp : mBreakPoints) {
    if (tell() >= bp.offset && tell() + sizeof(T) <= bp.offset + bp.size) {
      printf("Reading from %04u (0x%04x) sized %u\n", tell(), tell(),
             (u32)sizeof(T));
      warnAt("Breakpoint hit", tell(), tell() + sizeof(T));
      __debugbreak();
    }
  }
#endif

  if (!unaligned)
    alignmentCheck(sizeof(T));

  const T* native_elem = reinterpret_cast<T*>(getStreamStart() + tell());
  return endianDecode<T, E>(*native_elem);
}
template <typename T, EndianSelect E, bool unaligned> T BinaryReader::read() {
  T decoded = peek<T, E, unaligned>();
  seek<Whence::Current>(sizeof(T));
  return decoded;
}

template <typename T, int num, EndianSelect E, bool unaligned>
std::array<T, num> BinaryReader::readX() {
  std::array<T, num> result;

  for (auto& e : result)
    e = read<T, E, unaligned>();

  return result;
}

// TODO: Can rewrite read/peek to use peekAt
template <typename T, EndianSelect E, bool unaligned>
T BinaryReader::peekAt(int trans) {
  if (!unaligned)
    boundsCheck(sizeof(T), tell() + trans);

#ifndef NDEBUG
  for (const auto& bp : mBreakPoints) {
    if (tell() + trans >= bp.offset &&
        tell() + trans + sizeof(T) <= bp.offset + bp.size) {
      printf("Reading from %04u (0x%04x) sized %u\n", (u32)tell() + trans,
             (u32)tell() + trans, (u32)sizeof(T));
      warnAt("Breakpoint hit", tell() + trans, tell() + trans + sizeof(T));
      rsl::debug_break();
    }
  }
#endif
  T decoded = endianDecode<T, E>(
      *reinterpret_cast<T*>(getStreamStart() + tell() + trans));

  return decoded;
}
template <typename T, EndianSelect E, bool unaligned>
rsl::expected<T, std::string> BinaryReader::tryGetAt(int trans) {
  if (!unaligned && (trans % sizeof(T))) {
    rsl::debug_break();
    return std::string("Alignment error: ") + std::to_string(tell()) +
           " is not " + std::to_string(sizeof(T)) + " byte aligned.";
  }

  if (trans < 0 || trans + sizeof(T) >= endpos()) {
    rsl::debug_break();
    return std::string("Bounds error: Writing ") + std::to_string(sizeof(T)) +
           "bytes to " + std::to_string(tell()) + " exceeds buffer size of " +
           std::to_string(endpos());
  }

#ifndef NDEBUG
  for (const auto& bp : mBreakPoints) {
    if (trans >= bp.offset && trans + sizeof(T) <= bp.offset + bp.size) {
      printf("Reading from %04u (0x%04x) sized %u\n", trans, trans,
             (unsigned)sizeof(T));
      warnAt("Breakpoint hit", trans, trans + sizeof(T));
      rsl::debug_break();
    }
  }
#endif
  T decoded =
      endianDecode<T, E>(*reinterpret_cast<T*>(getStreamStart() + trans));

  return decoded;
}

template <typename lastReadType, typename TInval>
void BinaryReader::signalInvalidityLast() {
  // Unfortunate workaround, would prefer to do warn<type>
  TInval::warn(*this, sizeof(lastReadType));
}
template <typename lastReadType, typename TInval, int>
void BinaryReader::signalInvalidityLast(const char* msg) {
  TInval::warn(*this, sizeof(lastReadType), msg);
}

template <u32 magic, bool critical> inline bool BinaryReader::expectMagic() {
  if (read<u32, EndianSelect::Big>() != magic) {
    signalInvalidityLast<u32, MagicInvalidity<magic>>();

    if (critical)
      exit(1);

    return false;
  }

  return true;
}

} // namespace oishii
