#pragma once

#include <LibCore/common.h>
#include <Lib3D/interface/i3dmodel.hpp>
#include <LibCube/GX/VertexTypes.hpp>

#include "IndexedPrimitive.hpp"
#include "VertexDescriptor.hpp"


namespace libcube {


struct IndexedPolygon : public lib3d::Polygon
{
	// In wii/gc, absolute indices across mprims
	u64 getNumPrimitives() const override
	{
		u64 total = 0;

		for (u64 i = 0; i < getNumMatrixPrimitives(); ++i)
			total += getMatrixPrimitiveNumIndexedPrimitive(i);

		return total;
	}
	// Triangles
	// We add this to the last mprim. May need to be split up later.
	s64 addPrimitive() override
	{
		if (getNumMatrixPrimitives() == 0)
			addMatrixPrimitive();

		const s64 idx = getNumMatrixPrimitives();
		assert(idx > 0);
		if (idx <= 0)
			return -1;

		//	const s64 iprim_idx = addMatrixPrimitiveIndexedPrimitive();
		//	assert(iprim_idx >= 0);
		//	if (iprim_idx < 0)
		//		return;
		//	
		//	auto& iprim = getMatrixPrimitiveIndexedPrimitive(idx, iprim_idx);
		//	
		//	// TODO
		return -1;
	}
	bool hasAttrib(SimpleAttrib attrib) const override
	{
		switch (attrib)
		{
		case SimpleAttrib::EnvelopeIndex:
			return getVcd()[gx::VertexAttribute::PositionNormalMatrixIndex];
		case SimpleAttrib::Position:
			return getVcd()[gx::VertexAttribute::Position];
		case SimpleAttrib::Normal:
			return getVcd()[gx::VertexAttribute::Position];
		case SimpleAttrib::Color0:
		case SimpleAttrib::Color1:
			return getVcd()[gx::VertexAttribute::Color0 + ((u64)attrib - (u64)SimpleAttrib::Color0)];
		case SimpleAttrib::TexCoord0:
		case SimpleAttrib::TexCoord1:
		case SimpleAttrib::TexCoord2:
		case SimpleAttrib::TexCoord3:
		case SimpleAttrib::TexCoord4:
		case SimpleAttrib::TexCoord5:
		case SimpleAttrib::TexCoord6:
		case SimpleAttrib::TexCoord7:
			return getVcd()[gx::VertexAttribute::TexCoord0 + ((u64)attrib - (u64)SimpleAttrib::TexCoord0)];
		default:
			return false;
		}
	}
	void setAttrib(SimpleAttrib attrib, bool v) override
	{
		// We usually recompute the index (no direct support*)
		switch (attrib)
		{
		case SimpleAttrib::EnvelopeIndex:
			getVcd().mAttributes[gx::VertexAttribute::PositionNormalMatrixIndex] = gx::VertexAttributeType::Direct;
			break;
		case SimpleAttrib::Position:
			getVcd().mAttributes[gx::VertexAttribute::Position] = gx::VertexAttributeType::Short;
			break;
		case SimpleAttrib::Normal:
			getVcd().mAttributes[gx::VertexAttribute::Position] = gx::VertexAttributeType::Short;
			break;
		case SimpleAttrib::Color0:
		case SimpleAttrib::Color1:
			getVcd().mAttributes[gx::VertexAttribute::Color0 + ((u64)attrib - (u64)SimpleAttrib::Color0)] = gx::VertexAttributeType::Short;
			break;
		case SimpleAttrib::TexCoord0:
		case SimpleAttrib::TexCoord1:
		case SimpleAttrib::TexCoord2:
		case SimpleAttrib::TexCoord3:
		case SimpleAttrib::TexCoord4:
		case SimpleAttrib::TexCoord5:
		case SimpleAttrib::TexCoord6:
		case SimpleAttrib::TexCoord7:
			getVcd().mAttributes[gx::VertexAttribute::TexCoord0 + ((u64)attrib - (u64)SimpleAttrib::TexCoord0)] = gx::VertexAttributeType::Short;
			break;
		default:
			assert(!"Invalid simple vertex attrib");
			break;
		}
	}
	IndexedPrimitive* getIndexedPrimitiveFromSuperIndex(u64 idx)
	{
		u64 cnt = 0;
		for (u64 i = 0; i < getNumMatrixPrimitives(); ++i)
		{
			if (idx >= cnt && idx < cnt + getMatrixPrimitiveNumIndexedPrimitive(i))
				return &getMatrixPrimitiveIndexedPrimitive(i, idx - cnt);

			cnt += getMatrixPrimitiveNumIndexedPrimitive(i);
		}
		return nullptr;
	}
	const IndexedPrimitive* getIndexedPrimitiveFromSuperIndex(u64 idx) const
	{
		u64 cnt = 0;
		for (u64 i = 0; i < getNumMatrixPrimitives(); ++i)
		{
			if (idx >= cnt && idx < cnt + getMatrixPrimitiveNumIndexedPrimitive(i))
				return &getMatrixPrimitiveIndexedPrimitive(i, idx - cnt);

			cnt += getMatrixPrimitiveNumIndexedPrimitive(i);
		}
		return nullptr;
	}
	u64 getPrimitiveVertexCount(u64 index) const override
	{
		const IndexedPrimitive* prim = getIndexedPrimitiveFromSuperIndex(index);
		assert(prim);
		return prim->mVertices.size();
	}
	void resizePrimitiveVertexArray(u64 index, u64 size) override
	{
		IndexedPrimitive* prim = getIndexedPrimitiveFromSuperIndex(index);
		assert(prim);
		prim->mVertices.resize(size);
	}
	SimpleVertex getPrimitiveVertex(u64 prim_idx, u64 vtx_idx) override
	{
		// const auto& iprim = getIndexedPrimitiveFromSuperIndex(prim_idx);
		// assert(vtx_idx < iprim.mVertices.size());
		// const auto& vtx = iprim.mVertices[vtx_idx];

		//return {
		//	vtx[gx::VertexAttribute::PositionNormalMatrixIndex],
		//	vtx[gx::VertexAttribute::Position]
		//};
		return {};
	}
	// We add on to the attached buffer
	void setPrimitiveVertex(u64 prim_idx, u64 vtx_idx, const SimpleVertex& vtx) override
	{

	}
	void update() override
	{
		// Split up added primitives if necessary
	}


	virtual u64 getNumMatrixPrimitives() const = 0;
	virtual s32 addMatrixPrimitive() = 0;
	virtual s16 getMatrixPrimitiveCurrentMatrix(u64 idx) const = 0;
	virtual void setMatrixPrimitiveCurrentMatrix(u64 idx, s16 mtx) = 0;
	// Matrix list access
	virtual u64 getMatrixPrimitiveNumIndexedPrimitive(u64 idx) const = 0;
	virtual const IndexedPrimitive& getMatrixPrimitiveIndexedPrimitive(u64 idx, u64 prim_idx) const = 0;
	virtual IndexedPrimitive& getMatrixPrimitiveIndexedPrimitive(u64 idx, u64 prim_idx) = 0;

	virtual VertexDescriptor& getVcd() = 0;
	virtual const VertexDescriptor& getVcd() const = 0;

};

}
