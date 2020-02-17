#pragma once

#include <LibCore/common.h>
#include <LibCube/Util/BoundBox.hpp>
#include <vector>
#include <LibCube/GX/VertexTypes.hpp>
#include <LibCube/Export/IndexedPolygon.hpp>


namespace libcube::jsystem {

struct J3DModel;

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
	PX_TYPE_INFO_EX("J3D Shape", "shape", "J::Shape", ICON_FA_VECTOR_SQUARE, ICON_FA_VECTOR_SQUARE);

	std::string getName() const override { return "Shape " + std::to_string(id); }
	void setName(const std::string& name) override { }
	u64 getNumMatrixPrimitives() const override
	{
		return mMatrixPrimitives.size();
	}
	s32 addMatrixPrimitive() override
	{
		mMatrixPrimitives.emplace_back();
		return (s32)mMatrixPrimitives.size() - 1;
	}
	s16 getMatrixPrimitiveCurrentMatrix(u64 idx) const override
	{
		assert(idx < mMatrixPrimitives.size());
		if (idx < mMatrixPrimitives.size())
			return mMatrixPrimitives[idx].mCurrentMatrix;
		else
			return -1;
	}
	void setMatrixPrimitiveCurrentMatrix(u64 idx, s16 mtx) override
	{
		assert(idx < mMatrixPrimitives.size());
		if (idx < mMatrixPrimitives.size())
			mMatrixPrimitives[idx].mCurrentMatrix = mtx;
	}
	// Matrix list access
	u64 getMatrixPrimitiveNumIndexedPrimitive(u64 idx) const override
	{
		assert(idx < mMatrixPrimitives.size());
		if (idx < mMatrixPrimitives.size())
			return mMatrixPrimitives[idx].mPrimitives.size();
		return 0;
	}
	const IndexedPrimitive& getMatrixPrimitiveIndexedPrimitive(u64 idx, u64 prim_idx) const override
	{
		assert(idx < mMatrixPrimitives.size());
		assert(prim_idx < mMatrixPrimitives[idx].mPrimitives.size());
		return mMatrixPrimitives[idx].mPrimitives[prim_idx];
	}
	IndexedPrimitive& getMatrixPrimitiveIndexedPrimitive(u64 idx, u64 prim_idx) override
	{
		assert(idx < mMatrixPrimitives.size());
		assert(prim_idx < mMatrixPrimitives[idx].mPrimitives.size());
		return mMatrixPrimitives[idx].mPrimitives[prim_idx];
	}
	virtual VertexDescriptor& getVcd() override
	{
		return mVertexDescriptor;
	}
	virtual const VertexDescriptor& getVcd() const override
	{
		return mVertexDescriptor;
	}
	lib3d::AABB getBounds() const override
	{
		return { bbox.m_minBounds, bbox.m_maxBounds };
	}

	glm::vec2 getUv(u64 chan, u64 id) const override;
	glm::vec4 getClr(u64 id) const override;
	glm::vec3 getPos(u64 id) const override;
	glm::vec3 getNrm(u64 id) const override;
	u64 addPos(const glm::vec3& v) override;
	u64 addNrm(const glm::vec3& v) override;
	u64 addClr(u64 chan, const glm::vec4& v) override;
	u64 addUv(u64 chan, const glm::vec2& v) override;

	void addTriangle(std::array<SimpleVertex, 3> tri) override;

};

}
