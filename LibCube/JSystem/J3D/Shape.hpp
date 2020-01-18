#pragma once

#include <LibRiiEditor/common.hpp>
#include <LibCube/Common/BoundBox.hpp>
#include <vector>
#include <LibCube/GX/VertexTypes.hpp>
#include <LibCube/Export/GCCollection.hpp>
namespace libcube::jsystem {



struct MatrixPrimitive
{
	s16 mCurrentMatrix = -1; // Part of the polygon in G3D

	std::vector<s16> mDrawMatrixIndices;

	std::vector<IndexedPrimitive> mPrimitives;

	MatrixPrimitive() = default;
	MatrixPrimitive(s16 current_matrix, std::vector<s16> drawMatrixIndices)
		: mCurrentMatrix(current_matrix), mDrawMatrixIndices(drawMatrixIndices)
	{}
};
struct ShapeData
{
	u32 id;
	enum class Mode
	{
		Normal,
		Billboard_XY,
		Billboard_Y,
		Skinned,

		Max
	};
	Mode mode;
	float bsphere;
	AABB bbox;

	

	std::vector<MatrixPrimitive> mMatrixPrimitives;
	VertexDescriptor mVertexDescriptor;
};
struct Shape final : public ShapeData, public IndexedPolygon
{
	u32 getNumMatrixPrimitives() const override
	{
		return mMatrixPrimitives.size();
	}
	s32 addMatrixPrimitive() override
	{
		mMatrixPrimitives.emplace_back();
		return mMatrixPrimitives.size() - 1;
	}
	s16 getMatrixPrimitiveCurrentMatrix(u32 idx) const override
	{
		assert(idx < mMatrixPrimitives.size());
		if (idx < mMatrixPrimitives.size())
			return mMatrixPrimitives[idx].mCurrentMatrix;
		else
			return -1;
	}
	void setMatrixPrimitiveCurrentMatrix(u32 idx, s16 mtx) override
	{
		assert(idx < mMatrixPrimitives.size());
		if (idx < mMatrixPrimitives.size())
			mMatrixPrimitives[idx].mCurrentMatrix = mtx;
	}
	// Matrix list access
	u32 getMatrixPrimitiveNumIndexedPrimitive(u32 idx) const override
	{
		assert(idx < mMatrixPrimitives.size());
		if (idx < mMatrixPrimitives.size())
			return mMatrixPrimitives[idx].mPrimitives.size();
	}
	const IndexedPrimitive& getMatrixPrimitiveIndexedPrimitive(u32 idx, u32 prim_idx) const override
	{
		assert(idx < mMatrixPrimitives.size());
		assert(prim_idx < mMatrixPrimitives[idx].mPrimitives.size());
		return mMatrixPrimitives[idx].mPrimitives[prim_idx];
	}
	IndexedPrimitive& getMatrixPrimitiveIndexedPrimitive(u32 idx, u32 prim_idx) override
	{
		assert(idx < mMatrixPrimitives.size());
		assert(prim_idx < mMatrixPrimitives[idx].mPrimitives.size());
		return mMatrixPrimitives[idx].mPrimitives[prim_idx];
	}
	virtual VertexDescriptor& getVcd() override
	{
		return mVertexDescriptor;
	}
	virtual const VertexDescriptor& getVcd() const
	{
		return mVertexDescriptor;
	}
};

}
