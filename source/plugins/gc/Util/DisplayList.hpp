#pragma once

#include <core/common.h>
#include <librii/gx.h>
#include <llvm/Support/Error.h>
#include <oishii/reader/binary_reader.hxx>

namespace libcube {

struct IMeshDLDelegate {
  virtual librii::gx::IndexedPrimitive&
  addIndexedPrimitive(gx::PrimitiveType type, u16 nVerts) = 0;
};

llvm::Error
DecodeMeshDisplayList(oishii::BinaryReader& reader, u32 start, u32 size,
                      IMeshDLDelegate& delegate,
                      const librii::gx::VertexDescriptor& descriptor,
                      std::map<gx::VertexBufferAttribute, u32>* optUsageMap);

} // namespace libcube
