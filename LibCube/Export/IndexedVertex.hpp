#pragma once

#include <LibCube/GX/VertexTypes.hpp>
#include <LibCore/common.h>

#include <array>

namespace libcube {

struct IndexedVertex
{
	inline const u16& operator[] (gx::VertexAttribute attr) const
	{
		assert((u64)attr < (u64)gx::VertexAttribute::Max);
		return indices[(u64)attr];
	}
	inline u16& operator[] (gx::VertexAttribute attr)
	{
		assert((u64)attr < (u64)gx::VertexAttribute::Max);
		return indices[(u64)attr];
	}

private:
	std::array<u16, (u64)gx::VertexAttribute::Max> indices;
};

}
