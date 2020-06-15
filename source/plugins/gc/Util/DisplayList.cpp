#include <plugins/gc/Util/DisplayList.hpp>

namespace libcube {

static llvm::Expected<u16> ProcessAttr(
    oishii::BinaryReader& reader,
    const std::map<gx::VertexAttribute, gx::VertexAttributeType>& attribstypes,
    gx::VertexAttribute a, u32 vi) {
  u16 val = 0;

  switch (attribstypes.at(a)) {
  case gx::VertexAttributeType::None:
    break;
  case gx::VertexAttributeType::Byte:
    val = reader.read<u8, oishii::EndianSelect::Current, true>();
    assert(val != 0xff);
    break;
  case gx::VertexAttributeType::Short:
    val = reader.read<u16, oishii::EndianSelect::Current, true>();
    if (val == 0xffff) {
      printf("Index: %u, Attribute: %x\n", vi, (u32)a);
      reader.warnAt("Disabled vertex", reader.tell() - 2, reader.tell());
    }
    assert(val != 0xffff);
    break;
  case gx::VertexAttributeType::Direct:
    if (a != gx::VertexAttribute::PositionNormalMatrixIndex &&
        a != gx::VertexAttribute::Texture0MatrixIndex &&
        a != gx::VertexAttribute::Texture1MatrixIndex) {

      return llvm::createStringError(std::errc::executable_format_error,
                                     "Direct vertex data is unsupported.");
    }
    // As PNM indices are always direct, we
    // still use them in an all-indexed vertex
    val = reader.read<u8, oishii::EndianSelect::Current, true>();
    assert(val != 0xff);
    break;
  default:
    return llvm::createStringError(std::errc::executable_format_error,
                                   "Unknown vertex attribute format.");
  }

  return val;
}

llvm::Error
DecodeMeshDisplayList(oishii::BinaryReader& reader, u32 start, u32 size,
                      IMeshDLDelegate& delegate,
                      const VertexDescriptor& descriptor,
                      std::map<gx::VertexBufferAttribute, u32>* optUsageMap) {
  oishii::Jump<oishii::Whence::Set> g(reader, start);

  const u32 end = reader.tell() + size;
  while (reader.tell() < end) {
    const u8 tag = reader.readUnaligned<u8>();

    // NOP
    if (tag == 0)
      continue;

    if ((tag & 0x80) == 0) {
      return llvm::createStringError(
          std::errc::executable_format_error,
          "Unexpected command in mesh display list.");
    }

    u16 nVerts = reader.readUnaligned<u16>();
    IndexedPrimitive& prim = delegate.addIndexedPrimitive(
        gx::DecodeDrawPrimitiveCommand(tag), nVerts);

    for (u16 vi = 0; vi < nVerts; ++vi) {
      for (int a = 0; a < (int)gx::VertexAttribute::Max; ++a) {
        if (descriptor.mBitfield & (1 << a)) {
          const auto attr = static_cast<gx::VertexAttribute>(a);
          const auto attr_buf = static_cast<gx::VertexBufferAttribute>(a);

          auto valOrErr = ProcessAttr(reader, descriptor.mAttributes, attr, vi);
          if (auto e = valOrErr.takeError())
            return e;

          const u16 val = *valOrErr;

          prim.mVertices[vi][attr] = val;

          if (optUsageMap && (*optUsageMap)[attr_buf] <= val)
            (*optUsageMap)[attr_buf] = val;
        }
      }
    }
  }

  return llvm::Error::success();
}

} // namespace libcube
