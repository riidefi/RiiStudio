#pragma once

#include <core/common.h>
#include <vector>
#include <plugins/gc/GX/VertexTypes.hpp>
#include <plugins/gc/Export/IndexedPolygon.hpp>

#include <core/kpi/Node.hpp>

namespace riistudio::g3d {

struct MatrixPrimitive
{
	std::vector<s16> mDrawMatrixIndices;
	std::vector<libcube::IndexedPrimitive> mPrimitives;

	MatrixPrimitive() = default;
	MatrixPrimitive(std::vector<s16> drawMatrixIndices)
		: mDrawMatrixIndices(drawMatrixIndices)
	{}

	bool operator==(const MatrixPrimitive& rhs) const {
		return mDrawMatrixIndices == rhs.mDrawMatrixIndices && mPrimitives == rhs.mPrimitives;
	}
};
struct PolygonData
{
	std::string mName;
	u32 mId;

	s16 mCurrentMatrix = -1; // Part of the polygon in G3D

	bool currentMatrixEmbedded = false;
	bool visible = true;

	// For IDs, set to -1 in binary to not exist. Here, empty string
	std::string mPositionBuffer;
	std::string mNormalBuffer;
	std::array<std::string, 2> mColorBuffer;
	std::array<std::string, 8> mTexCoordBuffer;

	std::vector<MatrixPrimitive> mMatrixPrimitives;
	libcube::VertexDescriptor mVertexDescriptor;

	bool operator==(const PolygonData& rhs) const {
		return mName == rhs.mName && mId == rhs.mId && mCurrentMatrix == rhs.mCurrentMatrix && currentMatrixEmbedded == rhs.currentMatrixEmbedded &&
			visible == rhs.visible && mPositionBuffer == rhs.mPositionBuffer && mNormalBuffer == rhs.mNormalBuffer && mColorBuffer == rhs.mColorBuffer &&
			mTexCoordBuffer == rhs.mTexCoordBuffer && mVertexDescriptor == rhs.mVertexDescriptor && mMatrixPrimitives == rhs.mMatrixPrimitives;
	}
};
struct Polygon : public PolygonData, public libcube::IndexedPolygon
{
	virtual const kpi::IDocumentNode* getParent() const { return nullptr; }
	std::string getName() const { return mName; }
	void setName(const std::string& name) override { mName = name; }

	u64 getNumMatrixPrimitives() const override {
		return mMatrixPrimitives.size();
	}
	s32 addMatrixPrimitive() override {
		mMatrixPrimitives.emplace_back();
		return (s32)mMatrixPrimitives.size() - 1;
	}
	s16 getMatrixPrimitiveCurrentMatrix(u64 idx) const override {
		return mCurrentMatrix;
	}
	void setMatrixPrimitiveCurrentMatrix(u64 idx, s16 mtx) override {
		mCurrentMatrix = mtx;
	}
	// Matrix list access
	u64 getMatrixPrimitiveNumIndexedPrimitive(u64 idx) const override
	{
		assert(idx < mMatrixPrimitives.size());
		if (idx < mMatrixPrimitives.size())
			return mMatrixPrimitives[idx].mPrimitives.size();
		return 0;
	}
	const libcube::IndexedPrimitive& getMatrixPrimitiveIndexedPrimitive(u64 idx, u64 prim_idx) const override
	{
		assert(idx < mMatrixPrimitives.size());
		assert(prim_idx < mMatrixPrimitives[idx].mPrimitives.size());
		return mMatrixPrimitives[idx].mPrimitives[prim_idx];
	}
	libcube::IndexedPrimitive& getMatrixPrimitiveIndexedPrimitive(u64 idx, u64 prim_idx) override
	{
		assert(idx < mMatrixPrimitives.size());
		assert(prim_idx < mMatrixPrimitives[idx].mPrimitives.size());
		return mMatrixPrimitives[idx].mPrimitives[prim_idx];
	}
	virtual libcube::VertexDescriptor& getVcd() override
	{
		return mVertexDescriptor;
	}
	virtual const libcube::VertexDescriptor& getVcd() const override
	{
		return mVertexDescriptor;
	}
	lib3d::AABB getBounds() const override
	{
		// TODO
		return { {0, 0, 0}, {0, 0, 0} };
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

	bool operator==(const Polygon& rhs) const {
		return PolygonData::operator==(rhs);
	}
};

}
