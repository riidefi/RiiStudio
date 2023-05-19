#pragma once

#include <bit>
#include <string>
#include <vector>

#include "../util/util.hxx"
#include "link.hxx"
#include "vector_writer.hxx"

namespace oishii {

class Writer final : public VectorWriter {
public:
  Writer(std::endian endian) : mFileEndian(endian) {}
  Writer(u32 buffer_size, std::endian endian)
      : VectorWriter(buffer_size), mFileEndian(endian) {}
  Writer(std::vector<u8>&& buf, std::endian endian)
      : VectorWriter(std::move(buf)), mFileEndian(endian) {}

  template <typename T, EndianSelect E = EndianSelect::Current>
  void transfer(T& out) {
    write<T>(out);
  }

  template <typename T, EndianSelect E = EndianSelect::Current>
  void write(T val, bool checkmatch = true) {
    using integral_t = integral_of_equal_size_t<T>;
    if (tell() > 200'000'000) {
      fprintf(stderr, "File size is astronomical");
      rsl::debug_break();
      abort();
    }
    while (tell() + sizeof(T) > mBuf.size())
      mBuf.push_back(0);

    breakPointProcess(sizeof(T));

    union {
      integral_t integral;
      T current;
    } raw;
    raw.current = val;
    const auto decoded = endianDecode<integral_t, E>(raw.integral);

#ifndef NDEBUG
    if (checkmatch && mDebugMatch.size() > tell() + sizeof(T)) {
      const auto before = *reinterpret_cast<integral_t*>(&mDebugMatch[tell()]);
      if (before != decoded && decoded != 0xcccccccc) {
        fprintf(stderr,
                "Matching violation at 0x%x: writing %x where should be %x\n",
                tell(), (u32)decoded, (u32)before);

        rsl::debug_break();
      }
    }
#endif

    *reinterpret_cast<integral_t*>(&mBuf[tell()]) = decoded;

    seek<Whence::Current>(sizeof(T));
  }

  template <typename T, EndianSelect E = EndianSelect::Current>
  inline void writeUnaligned(T value) {
    write<T, E>(value);
  }
  template <EndianSelect E = EndianSelect::Current>
  void writeN(std::size_t sz, u32 val) {
    if (tell() > 200'000'000) {
      fprintf(stderr, "File size is astronomical");
      rsl::debug_break();
      abort();
    }
    while (tell() + sz > mBuf.size())
      mBuf.push_back(0);

    u32 decoded = endianDecode<u32, E>(val);

#if 0
#ifndef NDEBUG1
		if (/*checkmatch && */mDebugMatch.size() > tell() + sz)
		{
			for (int i = 0; i < sz; ++i)
			{
				const auto before = mDebugMatch[tell() + i];
				if (before != decoded >> (8 * i))
				{
					printf("Matching violation at %x: writing %x where should be %x\n", tell(), decoded, before);
					rsl::debug_break();
				}
			}
		}
#endif
#endif
    for (int i = 0; i < sz; ++i)
      mBuf[tell() + i] = static_cast<u8>(decoded >> (8 * i));

    seek<Whence::Current>(sz);
  }

  std::string mNameSpace = ""; // set by linker, stored in reservations
  std::string mBlockName = ""; // set by linker, stored in reservations

  struct ReferenceEntry {
    std::size_t addr;  //!< Address in writer stream.
    std::size_t TSize; //!< Size of link type
    Link mLink;        //!< The link.

    std::string nameSpace; //!< Namespace
    std::string blockName; //!< Necessary for child namespace lookup
  };
  std::vector<ReferenceEntry> mLinkReservations; // To be resolved by linker

  template <typename T> void writeLink(const Link& link) {
    // Add our resolvement reservation
    mLinkReservations.push_back(
        {tell(), sizeof(T), link, mNameSpace, mBlockName}); // copy

    // Dummy data
    write<T>(
        static_cast<T>(0xcccccccc)); // TODO: We don't want to cast for floats
  }

  template <typename T> void writeLink(const Hook& from, const Hook& to) {
    writeLink<T>(Link{from, to});
  }

  void setEndian(std::endian endian) noexcept { mFileEndian = endian; }
  bool getIsBigEndian() const noexcept {
    return mFileEndian == std::endian::big;
  }

  template <typename T, EndianSelect E>
  inline T endianDecode(T val) const noexcept {
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

  constexpr u32 roundDown(u32 in, u32 align) {
    return align ? in & ~(align - 1) : in;
  }
  constexpr u32 roundUp(u32 in, u32 align) {
    return align ? roundDown(in + (align - 1), align) : in;
  }

  void alignTo(u32 alignment) {
    auto pad_begin = tell();
    auto pad_end = roundUp(tell(), alignment);
    if (pad_begin == pad_end)
      return;
    for (u32 i = 0; i < pad_end - pad_begin; ++i)
      this->write<u8>(0);
    if (mUserPad)
      mUserPad(reinterpret_cast<char*>(getDataBlockStart()) + pad_begin,
               pad_end - pad_begin);
  }
  using PadFunction = void (*)(char* dst, u32 size);
  PadFunction mUserPad = nullptr;

  template <typename T> void writeAt(T val, u32 pos) {
    const auto back = tell();
    seekSet(pos);
    write<T>(val);
    seekSet(back);
  }

  inline u32 reserveNext(s32 n) {
    assert(n > 0);
    if (n == 0)
      return tell();

    const auto start = tell();

    skip(n - 1);
    write<u8>(0, false);
    skip(-n);
    assert(tell() == start);

    return start;
  }

  void saveToDisk(std::string_view path) const { FlushFile(mBuf, path); }

private:
  std::endian mFileEndian = std::endian::big; // to swap
};

inline auto writePlaceholder(oishii::Writer& writer) {
  writer.write<s32>(0, /* checkmatch */ false);
  return writer.tell() - 4;
}
inline void writeOffsetBackpatch(oishii::Writer& w, std::size_t pointer,
                                 std::size_t from) {
  auto old = w.tell();
  w.seekSet(pointer);
  w.write<s32>(old - from);
  w.seekSet(old);
}

} // namespace oishii
