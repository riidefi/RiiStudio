#include "Model.hpp"
#include "Collection.hpp"

namespace libcube::jsystem {

glm::vec2 Shape::getUv(u64 chan, u64 idx) const
{
	auto& mMdl = dynamic_cast<J3DCollection*>(mpScene)->getModels()[0];

	if (idx > mMdl.mBufs.uv[chan].mData.size())
		return {};
	return mMdl.mBufs.uv[chan].mData[idx];
}
glm::vec3 Shape::getPos(u64 idx) const
{
	auto& mMdl = dynamic_cast<J3DCollection*>(mpScene)->getModels()[0];
	return mMdl.mBufs.pos.mData[idx];
}
glm::vec3 Shape::getNrm(u64 idx) const
{
	auto& mMdl = dynamic_cast<J3DCollection*>(mpScene)->getModels()[0];
	return mMdl.mBufs.norm.mData[idx];
}
glm::vec4 Shape::getClr(u64 idx) const
{
	auto& mMdl = dynamic_cast<J3DCollection*>(mpScene)->getModels()[0];
	if (idx >= mMdl.mBufs.color[0].mData.size())
		return { 1, 1, 1, 1 };

	auto raw = static_cast<gx::ColorF32>(mMdl.mBufs.color[0].mData[idx]);

	return { raw.r, raw.g, raw.b, raw.a };
}
template<typename T>
static u64 addUnique(std::vector<T>& array, const T& value)
{
	//	const auto found = std::find(array.begin(), array.end(), value);
	//	if (found != array.end())
	//		return found - array.begin();
	const auto idx = array.size();
	array.push_back(value);
	return idx;
}

u64 Shape::addPos(const glm::vec3& v)
{
	assert(mpScene);
	auto& mMdl = dynamic_cast<J3DCollection*>(mpScene)->getModels()[0];
	u64 out = addUnique(mMdl.mBufs.pos.mData, v);
	return out;
}
u64 Shape::addNrm(const glm::vec3& v)
{
	auto& mMdl = dynamic_cast<J3DCollection*>(mpScene)->getModels()[0];
	return addUnique(mMdl.mBufs.norm.mData, v);
}
u64 Shape::addClr(u64 chan, const glm::vec4& v)
{
	auto& mMdl = dynamic_cast<J3DCollection*>(mpScene)->getModels()[0];
	// TODO: Round
	return addUnique(mMdl.mBufs.color[chan].mData, gx::Color{
		static_cast<u8>(v[0] * 255.0f),
		static_cast<u8>(v[1] * 255.0f),
		static_cast<u8>(v[2] * 255.0f),
		static_cast<u8>(v[3] * 255.0f) });
}
u64 Shape::addUv(u64 chan, const glm::vec2& v)
{
	auto& mMdl = dynamic_cast<J3DCollection*>(mpScene)->getModels()[0];
	return addUnique(mMdl.mBufs.uv[chan].mData, v);
}

void Shape::addTriangle(std::array<SimpleVertex, 3> tri)
{
	if (mMatrixPrimitives.empty())
		mMatrixPrimitives.emplace_back();
	if (mMatrixPrimitives.back().mPrimitives.empty() || mMatrixPrimitives.back().mPrimitives.back().mType != gx::PrimitiveType::Triangles)
		mMatrixPrimitives.back().mPrimitives.emplace_back(gx::PrimitiveType::Triangles, 0);

	auto& prim = mMatrixPrimitives.back().mPrimitives.back();

	for (const auto& vtx : tri)
	{
		// assert(!hasAttrib(SimpleAttrib::EnvelopeIndex));

		IndexedVertex ivtx;


		if (hasAttrib(SimpleAttrib::Position))
			ivtx[gx::VertexAttribute::Position] = addPos(vtx.position);
		if (hasAttrib(SimpleAttrib::Normal))
			ivtx[gx::VertexAttribute::Normal] = addNrm(vtx.normal);
		if (hasAttrib(SimpleAttrib::Color0))
			ivtx[gx::VertexAttribute::Color0] = addClr(0, vtx.colors[0]);
		if (hasAttrib(SimpleAttrib::Color1))
			ivtx[gx::VertexAttribute::Color1] = addClr(1, vtx.colors[1]);
		// TODO: Support all
		if (hasAttrib(SimpleAttrib::TexCoord0))
			ivtx[gx::VertexAttribute::TexCoord0] = addUv(0, vtx.uvs[0]);
		if (hasAttrib(SimpleAttrib::TexCoord1))
			ivtx[gx::VertexAttribute::TexCoord1] = addUv(1, vtx.uvs[1]);

		prim.mVertices.push_back(ivtx);
	}
}
}
