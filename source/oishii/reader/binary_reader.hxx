#pragma once

#include "../interfaces.hxx"
#include "../util/util.hxx"
#include <array> // Why?
#include <array>
#include <memory>
#include <string>
#include <vector>

namespace oishii {

// Trait
struct Invalidity {
  enum { invality_t };
};

class BinaryReader final : public IReader {
public:
  BinaryReader(u32 buffer_size, const char* file_name = "")
      : mBuf(buffer_size), mBufSize(buffer_size), file(file_name) {}
  BinaryReader(std::vector<u8> buffer, u32 buffer_size,
               const char* file_name = "")
      : mBuf(std::move(buffer)), mBufSize(buffer_size), file(file_name) {}
  ~BinaryReader() = default;

  // MemoryBlockReader
  u32 tell() final override { return mPos; }
  void seekSet(u32 ofs) final override { mPos = ofs; }
  u32 startpos() final override { return 0; }
  u32 endpos() final override { return mBufSize; }
  u8* getStreamStart() final override { return mBuf.data(); }

  // Faster bound check
  inline bool isInBounds(u32 pos) {
    // Can't be negative, start always at zero
    return pos < mBufSize;
  }

private:
  u32 mPos = 0;
  std::vector<u8> mBuf;
  u32 mBufSize;

public:
  //! @brief Given a type T, return T in the specified endiannes. (Current: swap
  //! endian if reader endian != sys endian)
  //!
  //! @tparam T Type of value to decode and return.
  //! @tparam E Endian transformation. Current will decode the value based on
  //! the endian switch flag
  //!
  //! @return T, endian decoded.
  //!
  template <typename T, EndianSelect E = EndianSelect::Current>
  inline T endianDecode(T val) const noexcept;

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
  template <u32 magic, bool critical = true> inline void expectMagic();

  void switchEndian() noexcept { bigEndian = !bigEndian; }
  void setEndian(bool big) noexcept { bigEndian = big; }
  bool getIsBigEndian() const noexcept { return bigEndian; }

  const char* getFile() const noexcept { return file; }
  void setFile(const char* f) noexcept { file = f; }

private:
  bool bigEndian = true; // to swap
  const char* file = "?";

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
      throw "Out of bounds read.";
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

} // namespace oishii

#include "binary_reader_impl.hxx"
#include "invalidities.hxx"
#include "stream_raii.hpp"
