#pragma once

#include <plugins/gc/GX/VertexTypes.hpp>
#include <core/common.h>

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
	bool operator==(const IndexedVertex& rhs) const {
		return indices == rhs.indices;
	}
private:
	std::array<u16, (u64)gx::VertexAttribute::Max> indices;
};

}
