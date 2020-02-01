#pragma once

#include <LibCube/GX/VertexTypes.hpp>
#include <LibCore/common.h>
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
};

} // namespace libcube
