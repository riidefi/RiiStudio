#pragma once

#include <LibCore/common.h>
#include <Lib3D/interface/i3dmodel.hpp>
#include <LibCube/GX/VertexTypes.hpp>

#include "IndexedPrimitive.hpp"
#include "VertexDescriptor.hpp"


namespace libcube {


struct IndexedPolygon : public lib3d::Polygon
{
	PX_TYPE_INFO("GameCube Polygon", "gc_indexedpoly", "GC::IndexedPolygon");
	// In wii/gc, absolute indices across mprims
	u64 getNumPrimitives() const;
	// Triangles
	// We add this to the last mprim. May need to be split up later.
	s64 addPrimitive();
	bool hasAttrib(SimpleAttrib attrib) const override;
	void setAttrib(SimpleAttrib attrib, bool v) override;
	IndexedPrimitive* getIndexedPrimitiveFromSuperIndex(u64 idx);
	const IndexedPrimitive* getIndexedPrimitiveFromSuperIndex(u64 idx) const;
	u64 getPrimitiveVertexCount(u64 index) const;
	void resizePrimitiveVertexArray(u64 index, u64 size);
	SimpleVertex getPrimitiveVertex(u64 prim_idx, u64 vtx_idx);
	void propogate(VBOBuilder& out) const override;
	virtual glm::vec3 getPos(u64 id) const = 0;
	virtual glm::vec3 getNrm(u64 id) const = 0;
	virtual glm::vec4 getClr(u64 id) const = 0;
	virtual glm::vec2 getUv(u64 chan, u64 id) const = 0;
	// We add on to the attached buffer
	void setPrimitiveVertex(u64 prim_idx, u64 vtx_idx, const SimpleVertex& vtx)
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
