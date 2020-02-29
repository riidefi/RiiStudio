#pragma once

#include <vector>
#include <string>

#include "vector_writer.hxx"
#include "../../util/util.hxx"
#include "link.hxx"

namespace oishii::v2 {

class Writer : public VectorWriter
{
public:
	Writer(u32 size)
		: VectorWriter(size)
	{}
	Writer(std::vector<u8> buf)
		: VectorWriter(std::move(buf))
	{}

	template<typename T, EndianSelect E = EndianSelect::Current>
	void transfer(T& out)
	{
		write<T>(out);
	}

	template<u32 size>
	struct integral_of_equal_size;

	template<>
	struct integral_of_equal_size<1> { using type = u8; };

	template<>
	struct integral_of_equal_size<2> { using type = u16; };

	template<>
	struct integral_of_equal_size<4> { using type = u32; };

	template<typename T>
	using integral_of_equal_size_t = typename integral_of_equal_size<sizeof(T)>::type;


	template <typename T, EndianSelect E = EndianSelect::Current>
	void write(T val, bool checkmatch=true)
	{
		using integral_t = integral_of_equal_size_t<T>;

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
		if (checkmatch && mDebugMatch.size() > tell() + sizeof(T))
		{
			const auto before = *reinterpret_cast<integral_t*>(&mDebugMatch[tell()]);
			if (before != decoded && decoded != 0xcccccccc)
			{
				printf("Matching violation at %x: writing %x where should be %x\n", tell(), (u32)decoded, (u32)before);
				__debugbreak();
			}
		}
#endif

		*reinterpret_cast<integral_t*>(&mBuf[tell()]) = decoded;

		seek<Whence::Current>(sizeof(T));
	}

	template <EndianSelect E = EndianSelect::Current>
	void writeN(std::size_t sz, u32 val)
	{
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
					__debugbreak();
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

	struct ReferenceEntry
	{
		std::size_t addr; //!< Address in writer stream.
		std::size_t TSize; //!< Size of link type
		Link mLink; //!< The link.

		std::string nameSpace; //!< Namespace
		std::string blockName; //!< Necessary for child namespace lookup
	};
	std::vector<ReferenceEntry> mLinkReservations; // To be resolved by linker

	template <typename T>
	void writeLink(const Link& link)
	{
		// Add our resolvement reservation
		mLinkReservations.push_back({ tell(), sizeof(T), link, mNameSpace, mBlockName }); // copy

		// Dummy data
		write<T>(static_cast<T>(0xcccccccc)); // TODO: We don't want to cast for floats
	}

	template<typename T>
	void writeLink(const Hook& from, const Hook& to)
	{
		writeLink<T>(Link{ from, to });
	}

	void switchEndian() noexcept { bigEndian = !bigEndian; }
	void setEndian(bool big)	noexcept { bigEndian = big; }
	bool getIsBigEndian() const noexcept { return bigEndian; }


	template <typename T, EndianSelect E>
	inline T endianDecode(T val) const noexcept
	{
		if (!Options::MULTIENDIAN_SUPPORT)
			return val;

		bool be = false;

		switch (E)
		{
		case EndianSelect::Current:
			be = bigEndian;
			break;
		case EndianSelect::Big:
			be = true;
			break;
		case EndianSelect::Little:
			be = false;
			break;
		}

		if (!Options::PLATFORM_LE)
			be = !be;

		return be ? swapEndian<T>(val) : val;
	}

	void alignTo(u32 alignment)
	{
		auto pad_begin = tell();
		while (tell() % alignment)
			write('F', false);
		if (pad_begin != tell() && mUserPad)
			mUserPad((char*)getDataBlockStart() + pad_begin, tell() - pad_begin);
	}
	using PadFunction = void(*)(char* dst, u32 size);
	PadFunction mUserPad = nullptr;
private:
	bool bigEndian = true; // to swap
};

}
