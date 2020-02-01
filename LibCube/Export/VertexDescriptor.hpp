#pragma once

#include <map>
#include <LibCore/common.h>
#include <LibCube/GX/VertexTypes.hpp>

namespace libcube {

struct VertexDescriptor
{
	std::map<gx::VertexAttribute, gx::VertexAttributeType> mAttributes;
	u32 mBitfield; // values of VertexDescriptor

	u64 getVcdSize() const
	{
		u64 vcd_size = 0;
		for (s64 i = 0; i < (s64)gx::VertexAttribute::Max; ++i)
			if (mBitfield & (1 << 1))
				++vcd_size;
		return vcd_size;
	}
	void calcVertexDescriptorFromAttributeList()
	{
		mBitfield = 0;
		for (gx::VertexAttribute i = gx::VertexAttribute::PositionNormalMatrixIndex;
			static_cast<u64>(i) < static_cast<u64>(gx::VertexAttribute::Max);
			i = static_cast<gx::VertexAttribute>(static_cast<u64>(i) + 1))
		{
			auto found = mAttributes.find(i);
			if (found != mAttributes.end() && found->second != gx::VertexAttributeType::None)
				mBitfield |= (1 << static_cast<u64>(i));
		}
	}
	bool operator[] (gx::VertexAttribute attr) const
	{
		return mBitfield & (1 << static_cast<u64>(attr));
	}
	bool operator==(const VertexDescriptor& rhs) const
	{
		// FIXME: remove NULL attribs before comparing
		return mAttributes.size() == rhs.mAttributes.size() &&
			std::equal(mAttributes.begin(), mAttributes.end(), rhs.mAttributes.begin());
	}
};

}
