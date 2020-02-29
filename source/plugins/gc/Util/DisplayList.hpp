#pragma once

#include <vendor/oishii/oishii/reader/binary_reader.hxx>
#include <core/common.h>
#include <plugins/gc/GX/VertexTypes.hpp>
#include <plugins/gc/Export/IndexedPrimitive.hpp>
#include <plugins/gc/Export/VertexDescriptor.hpp>

namespace libcube {

struct IMeshDLDelegate
{
	virtual IndexedPrimitive& addIndexedPrimitive(gx::PrimitiveType type, u16 nVerts) = 0;
};

void DecodeMeshDisplayList(oishii::BinaryReader& reader, u32 start, u32 size, IMeshDLDelegate& delegate, const VertexDescriptor& descriptor, std::map<gx::VertexBufferAttribute, u32>* optUsageMap);

} // namespace libcube
