#pragma once

#include "../data_provider.hxx"
#include "../interfaces.hxx"
#include "../util/util.hxx"

#include <core/common.h>
#include <rsl/DebugBreak.hpp>

namespace oishii {

// Trait
struct Invalidity {
  enum { invality_t };
};

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

//! Distributes error messages to all connected handlers.
//!
struct ErrorEmitter {
public:
  ErrorEmitter(const DataProvider& provider) : mProvider(&provider) {}
  ~ErrorEmitter() = default;

  void addErrorHandler(ErrorHandler* handler) {
    mErrorHandlers.emplace(handler);
  }
  void removeErrorHandler(ErrorHandler* handler) {
    mErrorHandlers.erase(handler);
  }

  void beginError() {
    for (auto* handler : mErrorHandlers)
      handler->onErrorBegin(*mProvider);
  }
  void describeError(const char* type, const char* brief, const char* details) {
    for (auto* handler : mErrorHandlers)
      handler->onErrorDescribe(*mProvider, type, brief, details);
  }
  void addErrorStackTrace(std::streampos start, std::streamsize size,
                          const char* domain) {
    for (auto* handler : mErrorHandlers)
      handler->onErrorAddStackTrace(*mProvider, start, size, domain);
  }
  void endError() {
    for (auto* handler : mErrorHandlers)
      handler->onErrorEnd(*mProvider);
  }

private:
  const DataProvider* mProvider = nullptr;
  std::set<ErrorHandler*> mErrorHandlers;
};

class BinaryReader final : public AbstractStream<BinaryReader>,
                           private ErrorEmitter {
public:
  BinaryReader(ByteView&& view);
  ~BinaryReader();

  void addErrorHandler(ErrorHandler* handler) {
    ErrorEmitter::addErrorHandler(handler);
  }
  void removeErrorHandler(ErrorHandler* handler) {
    ErrorEmitter::removeErrorHandler(handler);
  }

  // MemoryBlockReader
  u32 tell() const { return mPos; }
  void seekSet(u32 ofs) { mPos = ofs; }
  u32 startpos() const { return 0; }
  u32 endpos() const { return static_cast<u32>(mView.size()); }
  const u8* getStreamStart() const { return mView.data(); }

  bool isInBounds(u32 pos) const { return mView.isInBounds(pos); }

  std::span<const u8> slice() const { return mView; }

private:
  u32 mPos = 0;
  ByteView mView;

public:
  template <typename T, EndianSelect E = EndianSelect::Current,
            bool unaligned = false>
  T peek();
  template <typename T, EndianSelect E = EndianSelect::Current,
            bool unaligned = false>
  T read();

  template <typename T, EndianSelect E = EndianSelect::Current,
            bool unaligned = false>
  std::expected<T, std::string> tryRead();

  template <typename T, EndianSelect E = EndianSelect::Current>
  inline T readUnaligned() {
    return read<T, E, true>();
  }

  template <typename T, int num = 1, EndianSelect E = EndianSelect::Current,
            bool unaligned = false>
  std::array<T, num> readX();

  template <typename T, EndianSelect E = EndianSelect::Current,
            bool unaligned = false>
  void transfer(T& out) {
    out = read<T, E, unaligned>();
  }

  template <typename T, EndianSelect E = EndianSelect::Current,
            bool unaligned = false>
  T peekAt(int trans);
  template <typename T, EndianSelect E = EndianSelect::Current,
            bool unaligned = false>
  std::expected<T, std::string> tryGetAt(int trans);
  //	template <typename T, EndianSelect E = EndianSelect::Current>
  //	T readAt();
  template <typename T, EndianSelect E = EndianSelect::Current,
            bool unaligned = false>
  T getAt(int trans) {
    return peekAt<T, E, unaligned>(trans - tell());
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

  auto createScoped(std::string&& region) {
    return ScopedRegion(*this, std::move(region));
  }

  //! @brief Warn that there is an issue in a certain range. Stack trace is read
  //! and reported.
  //!
  //! @param[in] msg			Message to print
  //! @param[in] selectBegin	File offset where warning selection begins.
  //! @param[in] selectEnd	File offset where warning selection ends.
  //! @param[in] checkStack	Whether or not to output a stack trace.
  //! Necessary to prevent infinite recursion.
  //!
  void warnAt(const char* msg, u32 selectBegin, u32 selectEnd,
              bool checkStack = true);

  template <typename lastReadType, typename TInval> void signalInvalidityLast();

  template <typename lastReadType, typename TInval,
            int = TInval::inval_use_userdata_string>
  void signalInvalidityLast(const char* userData);

  // Magics are assumed to be 32 bit
  template <u32 magic, bool critical = true> inline bool expectMagic();

  void setEndian(std::endian endian) noexcept { mFileEndian = endian; }
  bool isBigEndian() const noexcept { return mFileEndian == std::endian::big; }

  // TODO
  const char* getFile() const noexcept {
    return mView.getProvider()->getFilePath().data();
  }

  void readBuffer(std::vector<u8>& out, u32 size, s32 ofs = -1) {
    if (ofs < 0)
      ofs = mPos;
    out.reserve(out.size() + size);
    boundsCheck(size);
    std::copy(mView.begin() + ofs, mView.begin() + ofs + size,
              std::back_inserter(out));
    mPos += size;
  }

  template <typename T>
  std::expected<std::vector<T>, std::string> tryReadBuffer(u32 size, u32 addr) {
    static_assert(sizeof(T) == 1);
    if (addr + size > endpos()) {
      rsl::debug_break();
      return std::unexpected("Buffer read exceeds file length");
    }
    readerBpCheck(size, addr - tell());
    std::vector<T> out(size);
    std::copy_n(mView.begin() + addr, size, out.begin());
    return out;
  }
  template <typename T>
  std::expected<std::vector<T>, std::string> tryReadBuffer(u32 size) {
    auto buf = tryReadBuffer<T>(size, tell());
    if (!buf) {
      return std::unexpected(buf.error());
    }
    seekSet(tell() + size);
    return *buf;
  }

private:
  std::endian mFileEndian = std::endian::big;

  void boundsCheck(u32 size, u32 at);
  void boundsCheck(u32 size) { boundsCheck(size, tell()); }
  void alignmentCheck(u32 size, u32 at);
  void alignmentCheck(u32 size) { alignmentCheck(size, tell()); }
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

#include "binary_reader_impl.hxx"
#include "invalidities.hxx"
#include "stream_raii.hpp"
