#pragma once

#include "binary_reader.hxx"

namespace oishii {

template<Whence W = Whence::Current, typename T = BinaryReader>
struct Jump
{
	inline Jump(T& stream, u32 offset)
		: mStream(stream), back(stream.tell())
	{
		mStream.template seek<W>(offset);
	}
	inline ~Jump()
	{
		mStream.template seek<Whence::Set>(back);
	}
	T& mStream;
	u32 back;
};

template<Whence W = Whence::Current, typename T = BinaryReader>
struct JumpOut
{
	inline JumpOut(T& stream, u32 offset)
		: mStream(stream), start(stream.tell()), back(offset)
	{}
	inline ~JumpOut()
	{
		mStream.template seek<Whence::Set>(start);
		mStream.template seek<W>(back);
	}
	T& mStream;

	u32 start;
	u32 back;
};

template<typename T = BinaryReader>
struct DebugExpectSized
#ifdef RELEASE
{
	DebugExpectSized(T& stream, u32 size) {}
	bool assertSince(u32) { return true; }
};
#else
{
	inline DebugExpectSized(T& stream, u32 size)
		: mStream(stream), mStart(stream.tell()), mSize(size)
	{}
	inline ~DebugExpectSized()
	{
		if (mStream.tell() - mStart != mSize)
			printf("Expected to read %u bytes -- instead read %u\n", mSize, mStream.tell() - mStart);
		assert(mStream.tell() - mStart == mSize && "Invalid size for this scope!");
	}

	bool assertSince(u32 dif)
	{
		const auto real_dif = mStream.tell() - mStart;

		if (real_dif != dif)
		{
			printf("Needed to read %u (%x) bytes; instead read %u (%x)\n", dif, dif, real_dif, real_dif);
			return false;
		}
		return true;
	}

	T& mStream;
	u32 mSize;
	u32 mStart;
};
#endif

} // namespace oishii
