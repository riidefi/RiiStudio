#pragma once

#include <librii/gx.h>
#include <map>
#include <oishii/reader/binary_reader.hxx>

namespace librii::gpu {

struct IMeshDLDelegate {
  virtual gx::IndexedPrimitive& addIndexedPrimitive(gx::PrimitiveType type,
                                                    u16 nVerts) = 0;
};

Result<void>
DecodeMeshDisplayList(oishii::BinaryReader& reader, u32 start, u32 size,
                      IMeshDLDelegate& delegate,
                      const gx::VertexDescriptor& descriptor,
                      std::map<gx::VertexBufferAttribute, u32>* optUsageMap);

} // namespace librii::gpu
