#pragma once

#include "../../interfaces.hxx"
#include <memory>
#include <vector>

namespace oishii::v2 {

//! @brief Writer with expanding buffer.
//!
class VectorWriter : public IWriter
{
public:
    VectorWriter(u32 buffer_size)
        : mPos(0), mBuf(buffer_size)
    {}
    VectorWriter(std::vector<u8> buf)
        : mPos(0), mBuf(std::move(buf))
    {}
    virtual ~VectorWriter() = default;

    // Faster to put it here (will call devirtualized functions)
	template<Whence W = Whence::Current>
	inline void seek(int ofs, u32 mAtPool = 0)
	{
		static_assert(W != Whence::Last, "Cannot use last seek yet.");
		static_assert(W == Whence::Set || W == Whence::Current || W == Whence::At, "Invalid whence.");
		switch (W)
		{
		case Whence::Set:
			mPos = ofs;
			break;
		case Whence::Current:
			if (ofs)
				mPos += ofs;
			break;
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
		return (u32)mBuf.size();
	}

	// Bound check unlike reader -- can always extend file
	inline bool isInBounds(u32 pos)
	{
		return pos < mBuf.size();
	}

	void attachDataForMatchingOutput(const std::vector<u8>& data)
	{
#ifndef NDEBUG
		mDebugMatch = data;
#endif
	}


protected:
	u32 mPos;
	std::vector<u8> mBuf;
#ifndef NDEBUG
	std::vector<u8> mDebugMatch;
#endif
public:
	void resize(u32 sz)
	{
        mBuf.resize(sz);
	}
	u8* getDataBlockStart()
	{
		return mBuf.data();
	}
	u32 getBufSize()
	{
		return (u32)mBuf.size();
	}
};

} // namespace oishii::v2
