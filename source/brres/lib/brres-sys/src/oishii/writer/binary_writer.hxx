#pragma once

#include <bit>
#include <string>
#include <vector>

#include "../Endian.hxx"
#include "link.hxx"
#include "vector_writer.hxx"

namespace oishii {

class Writer final : public VectorWriter {
public:
  Writer(std::endian endian);
  Writer(uint32_t buffer_size, std::endian endian);
  Writer(std::vector<u8>&& buf, std::endian endian);

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
    const auto decoded = endianDecode<integral_t, E>(raw.integral, m_endian);

#ifndef NDEBUG
    if (checkmatch && mDebugMatch.size() > tell() + sizeof(T)) {
      const auto before = *reinterpret_cast<integral_t*>(&mDebugMatch[tell()]);
      if (before != decoded && decoded != 0xcccccccc) {
        fprintf(stderr,
                "Matching violation at 0x%x: writing %x where should be %x\n",
                tell(), (uint32_t)decoded, (uint32_t)before);

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
  void writeN(std::size_t sz, uint32_t val) {
    if (tell() > 200'000'000) {
      fprintf(stderr, "File size is astronomical");
      rsl::debug_break();
      abort();
    }
    while (tell() + sz > mBuf.size())
      mBuf.push_back(0);

    uint32_t decoded = endianDecode<uint32_t, E>(val);

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

  void setEndian(std::endian endian) noexcept { m_endian = endian; }
  std::endian endian() const noexcept { return m_endian; }

  void alignTo(uint32_t alignment) {
    auto pad_begin = tell();
    auto pad_end = roundUp(tell(), alignment);
    if (pad_begin == pad_end)
      return;
    for (uint32_t i = 0; i < pad_end - pad_begin; ++i)
      this->write<u8>(0);
    if (mUserPad)
      mUserPad(reinterpret_cast<char*>(getDataBlockStart()) + pad_begin,
               pad_end - pad_begin);
  }
  using PadFunction = void (*)(char* dst, uint32_t size);
  PadFunction mUserPad = nullptr;

  template <typename T> void writeAt(T val, uint32_t pos) {
    const auto back = tell();
    seekSet(pos);
    write<T>(val);
    seekSet(back);
  }

  uint32_t reserveNext(int32_t n);
  void saveToDisk(std::string_view path) const;
  void breakPointProcess(uint32_t tell, uint32_t size);
  void breakPointProcess(uint32_t size);

private:
  std::endian m_endian = std::endian::big; // to swap
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
