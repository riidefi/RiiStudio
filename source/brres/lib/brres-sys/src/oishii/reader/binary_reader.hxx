#pragma once

#include "../Endian.hxx"
#include "../VectorStream.hxx"
#include "../interfaces.hxx"
#include <rsl/DebugBreak.hpp>
#include <rsl/Expected.hpp>
#include <rsl/Format.hpp>

// HACK
extern bool gTestMode;

namespace oishii {

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
  std::endian endian() const { return m_endian; }
  void setEndian(std::endian endian) noexcept { m_endian = endian; }

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
            uint32_t n>
  auto tryReadX() -> Result<std::array<T, n>> {
    std::array<T, n> result;
    for (auto& r : result) {
      auto tmp = tryRead<T>();
      if (!tmp) {
        return RSL_UNEXPECTED(tmp.error());
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
      auto err = std::format(
          "Alignment error: 0x{:x} ({} decimal) is not {}-byte aligned.", trans,
          trans, sizeof(T));
      if (gTestMode) {
        fprintf(stderr, "%s\n", err.c_str());
        rsl::debug_break();
      }
      return RSL_UNEXPECTED(err);
    }

    if (trans < 0 || trans + sizeof(T) > endpos()) {
      auto err = std::format("Bounds error: Reading {} bytes from 0x{:} ({} "
                             "decimal) exceeds buffer size of 0x{:x} ({})",
                             sizeof(T), trans, trans, endpos(), endpos());
      if (gTestMode) {
        fprintf(stderr, "%s\n", err.c_str());
        rsl::debug_break();
      }
      return RSL_UNEXPECTED(err);
    }

    readerBpCheck(sizeof(T), trans - tell());
    T decoded = endianDecode<T, E>(
        *reinterpret_cast<const T*>(getStreamStart() + trans), m_endian);

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
    uint32_t jump_save = 0;
    uint32_t jump_size_save = 0;

  public:
    uint32_t start = 0;
    BinaryReader& mReader;
  };

  //! Create a debug frame
  auto createScoped(std::string&& region) {
    return ScopedRegion(*this, std::move(region));
  }

  //! Print a warning message
  void warnAt(const char* msg, uint32_t selectBegin, uint32_t selectEnd,
              bool checkStack = true);

  template <typename T>
  auto tryReadBuffer(uint32_t size, uint32_t addr) -> Result<std::vector<T>> {
    static_assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4);
    if (addr + size * sizeof(T) > endpos()) {
      auto err = std::format("Bounds error: Reading {} bytes from 0x{:} ({} "
                             "decimal) exceeds buffer size of 0x{:x} ({})",
                             sizeof(T) * size, addr, addr, endpos(), endpos());
      rsl::debug_break();
      return RSL_UNEXPECTED(err);
    }
    readerBpCheck(size, addr - tell());
    if constexpr (sizeof(T) == 1) {
      std::vector<T> out(size);
      std::copy_n(mBuf.begin() + addr, size, out.begin());
      return out;
    }
    std::vector<T> out(size);
    auto it = addr;
    for (auto& t : out) {
      t = TRY(tryGetAt<T>(it));
      it += sizeof(T);
    }
    return out;
  }
  template <typename T>
  auto tryReadBuffer(uint32_t size) -> Result<std::vector<T>> {
    auto buf = tryReadBuffer<T>(size, tell());
    if (!buf) {
      return RSL_UNEXPECTED(buf.error());
    }
    seekSet(tell() + size);
    return *buf;
  }

private:
  std::endian m_endian = std::endian::big;
  std::string m_path = "Unknown Path";

  void readerBpCheck(uint32_t size, s32 trans = 0);

  struct DispatchStack;
  std::unique_ptr<DispatchStack> mStack;
  void enterRegion(std::string&& name, uint32_t& jump_save,
                   uint32_t& jump_size_save, uint32_t start, uint32_t size);
  void exitRegion(uint32_t jump_save, uint32_t jump_size_save);
};

inline std::span<const u8> SliceStream(oishii::BinaryReader& reader) {
  return {reader.getStreamStart() + reader.tell(),
          reader.endpos() - reader.tell()};
}

} // namespace oishii

#include "stream_raii.hxx"
