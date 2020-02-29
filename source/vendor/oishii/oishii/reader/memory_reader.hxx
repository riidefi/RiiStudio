#pragma once

#include "../interfaces.hxx"

#include <memory>
#include <array> // Why?
#include <vector>

namespace oishii {
class MemoryBlockReader : public IReader
{
public:
	MemoryBlockReader(u32 sz)
		: mPos(0), mBufSize(sz), mBuf(std::vector<u8>(sz))
	{}
	MemoryBlockReader(std::vector<u8> buf, u32 sz)
		: mPos(0), mBufSize(sz), mBuf(std::move(buf))
	{}
	virtual ~MemoryBlockReader() {}

	// Faster version for memoryblock
	template<Whence W = Whence::Current>
	inline void seek(int ofs, u32 mAtPool=0)
	{
		static_assert(W != Whence::Last, "Cannot use last seek yet.");
		static_assert(W == Whence::Set || W == Whence::Current || W == Whence::End || W == Whence::At, "Invalid whence.");
		switch (W)
		{
		case Whence::Set:
			mPos = ofs;
			break;
		case Whence::Current:
			if (ofs)
				mPos += ofs;
			break;
			//case Whence::End:
			//	// TODO: Is it endpos - pos or endpos + pos
			//	seekSet(endpos() - ofs);
			//	break;
		// At hack: summative
		case Whence::At:
			mPos = ofs + mAtPool;
			break;
		}
	}

	u32 tell() final override
	{
		return mPos;
	}
	void seekSet(u32 ofs) final override
	{
		mPos = ofs;
	}
	u32 startpos() final override
	{
		return 0;
	}
	u32 endpos() final override
	{
		return mBufSize;
	}
	u8* getStreamStart() final override
	{
		return mBuf.data();
	}

	// Faster bound check
	inline bool isInBounds(u32 pos)
	{
		// Can't be negative, start always at zero
		return pos < mBufSize;
	}

private:
	u32 mPos;
	u32 mBufSize;
	std::vector<u8> mBuf;
};

} // namespace oishii
