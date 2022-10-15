#pragma once

#include "../data_provider.hxx"
#include "../interfaces.hxx"
#include "../util/util.hxx"
#include <array>
#include <bit>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace oishii {

// Trait
struct Invalidity {
  enum { invality_t };
};

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
  BinaryReader(ByteView&& view)
      : ErrorEmitter(*view.getProvider()), mView(std::move(view)) {}
  ~BinaryReader() = default;

  void addErrorHandler(ErrorHandler* handler) {
    ErrorEmitter::addErrorHandler(handler);
  }
  void removeErrorHandler(ErrorHandler* handler) {
    ErrorEmitter::removeErrorHandler(handler);
  }

  // MemoryBlockReader
  u32 tell() { return mPos; }
  void seekSet(u32 ofs) { mPos = ofs; }
  u32 startpos() { return 0; }
  u32 endpos() { return mView.size(); }
  u8* getStreamStart() { return (u8*)mView.data(); }

  bool isInBounds(u32 pos) { return mView.isInBounds(pos); }

private:
  u32 mPos = 0;
  ByteView mView;

public:
  template <typename T, EndianSelect E = EndianSelect::Current>
  T endianDecode(T val) const {
    if constexpr (E == EndianSelect::Big) {
      return std::endian::native != std::endian::big ? swapEndian<T>(val) : val;
    } else if constexpr (E == EndianSelect::Little) {
      return std::endian::native != std::endian::little ? swapEndian<T>(val)
                                                        : val;
    } else if constexpr (E == EndianSelect::Current) {
      return std::endian::native != mFileEndian ? swapEndian<T>(val) : val;
    }

    return val;
  }

  template <typename T, EndianSelect E = EndianSelect::Current,
            bool unaligned = false>
  T peek();
  template <typename T, EndianSelect E = EndianSelect::Current,
            bool unaligned = false>
  T read();
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
  //	template <typename T, EndianSelect E = EndianSelect::Current>
  //	T readAt();
  template <typename T, EndianSelect E = EndianSelect::Current,
            bool unaligned = false>
  T getAt(int trans) {
    return peekAt<T, E, unaligned>(trans - tell());
  }

  struct ScopedRegion {
    ScopedRegion(BinaryReader& reader, const char* name) : mReader(reader) {
      start = reader.tell();
      mReader.enterRegion(name, jump_save, jump_size_save, reader.tell(), 0);
    }
    ~ScopedRegion() { mReader.exitRegion(jump_save, jump_size_save); }

  private:
    u32 jump_save = 0;
    u32 jump_size_save = 0;

  public:
    u32 start = 0;
    BinaryReader& mReader;
  };

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
  bool getIsBigEndian() const noexcept {
    return mFileEndian == std::endian::big;
  }

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

private:
  std::endian mFileEndian = std::endian::big;

  struct DispatchStack {
    struct Entry {
      u32 jump; // Offset in stream where jumped
      u32 jump_sz;

      const char* handlerName; // Name of handler
      u32 handlerStart;        // Start address of handler
    };

    std::array<Entry, 16> mStack;
    u32 mSize = 0;

    void push_entry(u32 j, const char* name, u32 start = 0) {
      Entry& cur = mStack[mSize];
      ++mSize;

      cur.jump = j;
      cur.handlerName = name;
      cur.handlerStart = start;
      cur.jump_sz = 1;
    }

    DispatchStack() = default;
  };

  DispatchStack mStack;

  void boundsCheck(u32 size, u32 at) {
    // TODO: Implement
    if (Options::BOUNDS_CHECK && at + size > endpos()) {
      // warnAt("Out of bounds read...", at, size + at);
      // Fatal invalidity -- out of space
      assert(!"Out of bounds read");
      abort();
      // throw "Out of bounds read.";
    }
  }
  void boundsCheck(u32 size) { boundsCheck(size, tell()); }
  void alignmentCheck(u32 size, u32 at) {
    if (Options::ALIGNMENT_CHECK && at % size) {
      // TODO: Filter warnings in same scope, only print stack once.
      warnAt((std::string("Alignment error: ") + std::to_string(tell()) +
              " is not " + std::to_string(size) + " byte aligned.")
                 .c_str(),
             at, at + size, true);
#ifdef RII_PLATFORM_WINDOWS
      __debugbreak();
#endif
    }
  }
  void alignmentCheck(u32 size) { alignmentCheck(size, tell()); }
  void enterRegion(const char* name, u32& jump_save, u32& jump_size_save,
                   u32 start, u32 size) {
    //_ASSERT(mStack.mSize < 16);
    mStack.push_entry(start, name, start);

    // Jump is owned by past block
    if (mStack.mSize > 1) {
      jump_save = mStack.mStack[mStack.mSize - 2].jump;
      jump_size_save = mStack.mStack[mStack.mSize - 2].jump_sz;
      mStack.mStack[mStack.mSize - 2].jump = start;
      mStack.mStack[mStack.mSize - 2].jump_sz = size;
    }
  }
  void exitRegion(u32 jump_save, u32 jump_size_save) {
    if (mStack.mSize > 1) {
      mStack.mStack[mStack.mSize - 2].jump = jump_save;
      mStack.mStack[mStack.mSize - 2].jump_sz = jump_size_save;
    }

    --mStack.mSize;
  }
};

inline std::span<const u8> SliceStream(oishii::BinaryReader& reader) {
  return {reader.getStreamStart() + reader.tell(),
          reader.endpos() - reader.tell()};
}

} // namespace oishii

#include "binary_reader_impl.hxx"
#include "invalidities.hxx"
#include "stream_raii.hpp"
