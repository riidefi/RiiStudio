#pragma once

#include "../interfaces.hxx"
#include <memory>
#include <vector>

namespace oishii {

//! @brief Writer with expanding buffer.
//!
class VectorWriter : public IWriter
{
public:
	VectorWriter(u32 sz)
		: mPos(0), mBuf(std::make_unique<std::vector<u8>>(sz))
	{}
	VectorWriter(std::unique_ptr<std::vector<u8>> buf, u32 sz)
		: mPos(0), mBuf(std::move(buf))
	{}
	virtual ~VectorWriter() {}

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
		return (u32)mBuf->size();
	}

	// Bound check unlike reader -- can always extend file
	inline bool isInBounds(u32 pos)
	{
		return pos < mBuf->size();
	}

protected:
	u32 mPos;
	std::unique_ptr<std::vector<u8>> mBuf;

public:
	void resize(u32 sz)
	{
		if (mBuf.get())
			mBuf->resize(sz);
	}
	u8* getDataBlockStart()
	{
		return mBuf.get() ? mBuf.get()->data() : nullptr;
	}
	u32 getBufSize()
	{
		return mBuf.get() ? (u32)mBuf->size() : 0;
	}
};

} // namespace oishii
