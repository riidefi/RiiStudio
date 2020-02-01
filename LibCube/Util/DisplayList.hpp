#pragma once

#include <oishii/reader/binary_reader.hxx>
#include <LibCore/common.h>
#include <LibCube/GX/VertexTypes.hpp>
#include <LibCube/Export/IndexedPrimitive.hpp>
#include <LibCube/Export/VertexDescriptor.hpp>

namespace libcube {

struct IMeshDLDelegate
{
	virtual IndexedPrimitive& addIndexedPrimitive(gx::PrimitiveType type, u16 nVerts) = 0;
};

void DecodeMeshDisplayList(oishii::BinaryReader& reader, u32 start, u32 size, IMeshDLDelegate& delegate, const VertexDescriptor& descriptor, std::map<gx::VertexBufferAttribute, u32>* optUsageMap);

} // namespace libcube
