#pragma once

#include <plugins/gc/GX/VertexTypes.hpp>
#include <core/common.h>
#include <vector>

#include "IndexedVertex.hpp"

namespace libcube {

struct IndexedPrimitive
{
	gx::PrimitiveType mType;
	std::vector<IndexedVertex> mVertices;

	IndexedPrimitive() = default;
	IndexedPrimitive(gx::PrimitiveType type, u64 size)
		: mType(type), mVertices(size)
	{
	}
	bool operator==(const IndexedPrimitive& rhs) const {
		return mType == rhs.mType && mVertices == rhs.mVertices;
	}
};

} // namespace libcube
