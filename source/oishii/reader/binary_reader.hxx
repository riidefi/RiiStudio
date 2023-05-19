#pragma once

#include "../VectorStream.hxx"
#include "../interfaces.hxx"
#include "../util/util.hxx"

#include <core/common.h>
#include <rsl/DebugBreak.hpp>

// HACK
extern bool gTestMode;

namespace oishii {

template <typename T, EndianSelect E = EndianSelect::Current>
inline T endianDecode(T val, std::endian fileEndian) {
  if constexpr (E == EndianSelect::Big) {
    return std::endian::native != std::endian::big ? swapEndian<T>(val) : val;
  } else if constexpr (E == EndianSelect::Little) {
    return std::endian::native != std::endian::little ? swapEndian<T>(val)
                                                      : val;
  } else if constexpr (E == EndianSelect::Current) {
    return std::endian::native != fileEndian ? swapEndian<T>(val) : val;
  }

  return val;
}

class BinaryReader final : public VectorStream {
public:
  //! Failure type is always `std::string`
  template <typename T> using Result = std::expected<T, std::string>;

  //! Read file from memory
  BinaryReader(std::vector<u8>&& view, std::string_view path,
               std::endian endian);
  BinaryReader(std::span<const u8> view, std::string_view path,
               std::endian endian);
  BinaryReader(const BinaryReader&) = delete;
  BinaryReader(BinaryReader&&);
  ~BinaryReader();

  //! Read file from disc
  static Result<BinaryReader> FromFilePath(std::string_view path,
                                           std::endian endian);

  // The |BinaryReader| keeps track of the files endianness
  std::endian endian() const { return mFileEndian; }
  void setEndian(std::endian endian) noexcept { mFileEndian = endian; }

  // Path of the file or "Unknown path"
  const char* getFile() const noexcept { return m_path.c_str(); }

  //! Get a read-only view of the file
  std::span<const u8> slice() const { return mBuf; }

  //! Pop a value from the stream (of type |T|)
  template <typename T,                             //
            EndianSelect E = EndianSelect::Current, //
            bool unaligned = false>
  Result<T> tryRead();

  //! Pop |n| values from the stream (each of type |T|)
  template <typename T, //
            u32 n>
  auto tryReadX() -> Result<std::array<T, n>> {
    std::array<T, n> result;
    for (auto& r : result) {
      auto tmp = tryRead<T>();
      if (!tmp) {
        return std::unexpected(tmp.error());
      }
      r = *tmp;
    }
    return result;
  }

  //! Get a value from an arbitrary point in the file
  template <typename T,                             //
            EndianSelect E = EndianSelect::Current, //
            bool unaligned = false>
  auto tryGetAt(int trans) -> Result<T> {
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

  struct ScopedRegion {
    ScopedRegion(BinaryReader& reader, std::string&& name) : mReader(reader) {
      start = reader.tell();
      mReader.enterRegion(std::move(name), jump_save, jump_size_save,
                          reader.tell(), 0);
    }
    ~ScopedRegion() { mReader.exitRegion(jump_save, jump_size_save); }

  private:
    u32 jump_save = 0;
    u32 jump_size_save = 0;

  public:
    u32 start = 0;
    BinaryReader& mReader;
  };

  //! Create a debug frame
  auto createScoped(std::string&& region) {
    return ScopedRegion(*this, std::move(region));
  }

  //! Print a warning message
  void warnAt(const char* msg, u32 selectBegin, u32 selectEnd,
              bool checkStack = true);

  template <typename T>
  auto tryReadBuffer(u32 size, u32 addr) -> Result<std::vector<T>> {
    static_assert(sizeof(T) == 1);
    if (addr + size > endpos()) {
      rsl::debug_break();
      return std::unexpected("Buffer read exceeds file length");
    }
    readerBpCheck(size, addr - tell());
    std::vector<T> out(size);
    std::copy_n(mBuf.begin() + addr, size, out.begin());
    return out;
  }
  template <typename T> auto tryReadBuffer(u32 size) -> Result<std::vector<T>> {
    auto buf = tryReadBuffer<T>(size, tell());
    if (!buf) {
      return std::unexpected(buf.error());
    }
    seekSet(tell() + size);
    return *buf;
  }

private:
  std::endian mFileEndian = std::endian::big;
  std::string m_path = "Unknown Path";

  void readerBpCheck(u32 size, s32 trans = 0);

  struct DispatchStack;
  std::unique_ptr<DispatchStack> mStack;
  void enterRegion(std::string&& name, u32& jump_save, u32& jump_size_save,
                   u32 start, u32 size);
  void exitRegion(u32 jump_save, u32 jump_size_save);
};

inline std::span<const u8> SliceStream(oishii::BinaryReader& reader) {
  return {reader.getStreamStart() + reader.tell(),
          reader.endpos() - reader.tell()};
}

} // namespace oishii

#include "stream_raii.hpp"
