#pragma once

#include "binary_reader.hxx"
#include <format>

// HACK
extern bool gTestMode;

namespace oishii {

template <u32 m> struct MagicInvalidity;

template <typename T, EndianSelect E, bool unaligned> T BinaryReader::peek() {
  boundsCheck(sizeof(T));

  readerBpCheck(sizeof(T));

  if (!unaligned)
    alignmentCheck(sizeof(T));

  const T* native_elem = reinterpret_cast<const T*>(getStreamStart() + tell());
  return endianDecode<T, E>(*native_elem, mFileEndian);
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
  boundsCheck(sizeof(T), tell() + trans);
  if (!unaligned)
    alignmentCheck(sizeof(T), tell() + trans);

  readerBpCheck(sizeof(T), trans);
  T decoded = endianDecode<T, E>(
      *reinterpret_cast<const T*>(getStreamStart() + tell() + trans),
      mFileEndian);

  return decoded;
}
template <typename T, EndianSelect E, bool unaligned>
std::expected<T, std::string> BinaryReader::tryGetAt(int trans) {
  if (!unaligned && (trans % sizeof(T))) {
    auto err = std::format("Alignment error: {} is not {}-byte aligned.",
                           tell(), sizeof(T));
    if (gTestMode) {
      fprintf(stderr, "%s\n", err.c_str());
      rsl::debug_break();
    }
    return std::unexpected(err);
  }

  if (trans < 0 || trans + sizeof(T) > endpos()) {
    auto err = std::format(
        "Bounds error: Reading {} bytes from {} exceeds buffer size of {}",
        sizeof(T), trans, endpos());
    if (gTestMode) {
      fprintf(stderr, "%s\n", err.c_str());
      rsl::debug_break();
    }
    return std::unexpected(err);
  }

  readerBpCheck(sizeof(T), trans - tell());
  T decoded = endianDecode<T, E>(
      *reinterpret_cast<const T*>(getStreamStart() + trans), mFileEndian);

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
