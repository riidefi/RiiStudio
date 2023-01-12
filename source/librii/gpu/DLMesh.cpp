#include "DLMesh.hpp"

namespace librii::gpu {

// This is always BE
constexpr oishii::EndianSelect CmdProcEndian = oishii::EndianSelect::Big;

static std::expected<u16, std::string> ProcessAttr(
    oishii::BinaryReader& reader,
    const std::map<gx::VertexAttribute, gx::VertexAttributeType>& attribstypes,
    gx::VertexAttribute a, u32 vi) {
  u16 val = 0;

  switch (attribstypes.at(a)) {
  case gx::VertexAttributeType::None:
    break;
  case gx::VertexAttributeType::Byte:
    val = TRY(reader.tryRead<u8, CmdProcEndian, true>());
    EXPECT(val != 0xff);
    break;
  case gx::VertexAttributeType::Short:
    val = TRY(reader.tryRead<u16, CmdProcEndian, true>());
    if (val == 0xffff) {
      printf("Index: %u, Attribute: %x\n", vi, (u32)a);
      reader.warnAt("Disabled vertex", reader.tell() - 2, reader.tell());
    }
    EXPECT(val != 0xffff);
    break;
  case gx::VertexAttributeType::Direct:
    if (a != gx::VertexAttribute::PositionNormalMatrixIndex &&
        a != gx::VertexAttribute::Texture0MatrixIndex &&
        a != gx::VertexAttribute::Texture1MatrixIndex) {

      return std::unexpected("Direct vertex data is unsupported.");
    }
    // As PNM indices are always direct, we
    // still use them in an all-indexed vertex
    val = TRY(reader.tryRead<u8, CmdProcEndian, true>());
    EXPECT(val != 0xff);
    break;
  default:
    return std::unexpected("Unknown vertex attribute format.");
  }

  return val;
}

Result<void>
DecodeMeshDisplayList(oishii::BinaryReader& reader, u32 start, u32 size,
                      IMeshDLDelegate& delegate,
                      const librii::gx::VertexDescriptor& descriptor,
                      std::map<gx::VertexBufferAttribute, u32>* optUsageMap) {
  oishii::Jump<oishii::Whence::Set> g(reader, start);

  const u32 end = reader.tell() + size;
  while (reader.tell() < end) {
    const u8 tag = TRY(reader.tryRead<u8, CmdProcEndian, true>());

    // NOP
    if (tag == 0)
      continue;

    if ((tag & 0x80) == 0) {
      return std::unexpected("Unexpected command in mesh display list.");
    }

    u16 nVerts = TRY(reader.tryRead<u16, CmdProcEndian, true>());
    auto& prim = delegate.addIndexedPrimitive(
        gx::DecodeDrawPrimitiveCommand(tag), nVerts);

    for (u16 vi = 0; vi < nVerts; ++vi) {
      for (int a = 0; a < (int)gx::VertexAttribute::Max; ++a) {
        if (descriptor.mBitfield & (1 << a)) {
          const auto attr = static_cast<gx::VertexAttribute>(a);
          const auto attr_buf = static_cast<gx::VertexBufferAttribute>(a);

          const u16 val =
              TRY(ProcessAttr(reader, descriptor.mAttributes, attr, vi));
          prim.mVertices[vi][attr] = val;

		  // TODO: Probably don't validate this here
		  if (a != (int)gx::VertexAttribute::PositionNormalMatrixIndex) {
            if (optUsageMap && (*optUsageMap)[attr_buf] <= val)
              (*optUsageMap)[attr_buf] = val;
          }
        }
      }
    }
  }

  return {};
}

} // namespace librii::gpu
