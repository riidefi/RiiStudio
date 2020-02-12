#include "Model.hpp"

namespace libcube::jsystem {

glm::vec2 Shape::getUv(u64 chan, u64 idx) const
{
	return mMdl.mBufs.uv[chan].mData[idx];
}
glm::vec3 Shape::getPos(u64 idx) const
{
	return mMdl.mBufs.pos.mData[idx];
}
glm::vec4 Shape::getClr(u64 idx) const
{
	if (idx >= mMdl.mBufs.color[0].mData.size())
		return {1, 1, 1, 1};

	auto raw = static_cast<gx::ColorF32>(mMdl.mBufs.color[0].mData[idx]);

	return { raw.r, raw.g, raw.b, raw.a };
}

}
