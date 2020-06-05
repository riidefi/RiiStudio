#pragma once

#include <core/common.h>
#include <oishii/reader/binary_reader.hxx>
#include <plugins/gc/Export/IndexedPrimitive.hpp>
#include <plugins/gc/Export/VertexDescriptor.hpp>
#include <plugins/gc/GX/VertexTypes.hpp>

namespace libcube {

struct IMeshDLDelegate {
  virtual IndexedPrimitive& addIndexedPrimitive(gx::PrimitiveType type,
                                                u16 nVerts) = 0;
};

void DecodeMeshDisplayList(
    oishii::BinaryReader& reader, u32 start, u32 size,
    IMeshDLDelegate& delegate, const VertexDescriptor& descriptor,
    std::map<gx::VertexBufferAttribute, u32>* optUsageMap);

} // namespace libcube
