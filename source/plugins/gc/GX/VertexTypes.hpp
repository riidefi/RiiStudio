#pragma once

#include <core/common.h>

#include <vendor/oishii/oishii/reader/binary_reader.hxx>

#include <plugins/gc/GX/Material.hpp>
#include <vendor/glm/vec3.hpp>

namespace libcube {
namespace gx {

union VertexComponentCount {

  enum class Position { xy, xyz } position;
  enum class Normal {
    xyz,
    nbt, // index NBT triplets
    nbt3 // index N/B/T individually
  } normal;
  enum class Color { rgb, rgba } color;
  enum class TextureCoordinate {
    s,
    st,

    u = s,
    uv = st
  } texcoord;

  explicit VertexComponentCount(Position p) : position(p) {}
  explicit VertexComponentCount(Normal n) : normal(n) {}
  explicit VertexComponentCount(Color c) : color(c) {}
  explicit VertexComponentCount(TextureCoordinate u) : texcoord(u) {}
  VertexComponentCount() {}
};

union VertexBufferType {
  enum class Generic { u8, s8, u16, s16, f32 } generic;
  enum class Color {
    rgb565, //!< R5G6B5
    rgb8,   //!< 8 bit, no alpha
    rgbx8,  //!< 8 bit, alpha discarded
    rgba4,  //!< 4 bit
    rgba6,  //!< 6 bit
    rgba8,  //!< 8 bit, alpha

    FORMAT_16B_565 = 0,
    FORMAT_24B_888 = 1,
    FORMAT_32B_888x = 2,
    FORMAT_16B_4444 = 3,
    FORMAT_24B_6666 = 4,
    FORMAT_32B_8888 = 5,
  } color;

  explicit VertexBufferType(Generic g) : generic(g) {}
  explicit VertexBufferType(Color c) : color(c) {}
  VertexBufferType() {}
};
enum class VertexAttribute : u32 {
  PositionNormalMatrixIndex = 0,
  Texture0MatrixIndex,
  Texture1MatrixIndex,
  Texture2MatrixIndex,
  Texture3MatrixIndex,
  Texture4MatrixIndex,
  Texture5MatrixIndex,
  Texture6MatrixIndex,
  Texture7MatrixIndex,
  Position,
  Normal,
  Color0,
  Color1,
  TexCoord0,
  TexCoord1,
  TexCoord2,
  TexCoord3,
  TexCoord4,
  TexCoord5,
  TexCoord6,
  TexCoord7,

  PositionMatrixArray,
  NormalMatrixArray,
  TextureMatrixArray,
  LightArray,
  NormalBinormalTangent,
  Max,

  Undefined = 0xff - 1,
  Terminate = 0xff,

};
// Subset of vertex attributes valid for a buffer.
enum class VertexBufferAttribute : u32 {
  Position = 9,
  Normal,
  Color0,
  Color1,
  TexCoord0,
  TexCoord1,
  TexCoord2,
  TexCoord3,
  TexCoord4,
  TexCoord5,
  TexCoord6,
  TexCoord7,

  NormalBinormalTangent = 25,
  Max,

  Undefined = 0xff - 1,
  Terminate = 0xff
};

enum class VertexAttributeType : u32 {
  None,   //!< No data is to be sent.
  Direct, //!< Data will be sent directly.
  Byte,   //!< 8-bit indices.
  Short   //!< 16-bit indices.
};

enum class PrimitiveType {
  Quads,         // 0x80
  Quads2,        // 0x88
  Triangles,     // 0x90
  TriangleStrip, // 0x98
  TriangleFan,   // 0xA0
  Lines,         // 0xA8
  LineStrip,     // 0xB0
  Points,        // 0xB8
  Max
};
constexpr u32 PrimitiveMask = 0x78;
constexpr u32 PrimitiveShift = 3;

constexpr u32 EncodeDrawPrimitiveCommand(PrimitiveType type) {
  return 0x80 | ((static_cast<u32>(type) << PrimitiveShift) & PrimitiveMask);
}
constexpr PrimitiveType DecodeDrawPrimitiveCommand(u32 cmd) {
  return static_cast<PrimitiveType>((cmd & PrimitiveMask) >> PrimitiveShift);
}

inline gx::VertexAttribute operator+(gx::VertexAttribute attr, u64 i) {
  u64 out = static_cast<u64>(attr) + i;
  assert(out < static_cast<u64>(gx::VertexAttribute::Max));
  return static_cast<gx::VertexAttribute>(out);
}

enum class VertexBufferKind { position, normal, color, textureCoordinate };

inline std::size_t computeComponentCount(gx::VertexBufferKind kind,
                                         gx::VertexComponentCount count) {
  switch (kind) {
  case VertexBufferKind::position:
    return static_cast<std::size_t>(count.position) + 2; // xy -> 2; xyz -> 3
  case VertexBufferKind::normal:
    return 3; // TODO: NBT, NBT3
  case VertexBufferKind::color:
    return static_cast<std::size_t>(count.color) + 3; // rgb -> 3; rgba -> 4
  case VertexBufferKind::textureCoordinate:
    return static_cast<std::size_t>(count.texcoord) + 1; // s -> 1, st -> 2
  default:
    throw "Invalid component count!";
  }
}

inline f32 readGenericComponentSingle(oishii::BinaryReader& reader,
                                      gx::VertexBufferType::Generic type,
                                      u32 divisor = 0) {
  switch (type) {
  case gx::VertexBufferType::Generic::u8:
    return static_cast<f32>(reader.read<u8>()) /
           static_cast<f32>((1 << divisor));
  case gx::VertexBufferType::Generic::s8:
    return static_cast<f32>(reader.read<s8>()) /
           static_cast<f32>((1 << divisor));
  case gx::VertexBufferType::Generic::u16:
    return static_cast<f32>(reader.read<u16>()) /
           static_cast<f32>((1 << divisor));
  case gx::VertexBufferType::Generic::s16:
    return static_cast<f32>(reader.read<s16>()) /
           static_cast<f32>((1 << divisor));
  case gx::VertexBufferType::Generic::f32:
    return reader.read<f32>();
  default:
    return 0.0f;
  }
}
inline gx::Color readColorComponents(oishii::BinaryReader& reader,
                                     gx::VertexBufferType::Color type) {
  gx::Color result; // TODO: Default color

  switch (type) {
  case gx::VertexBufferType::Color::rgb565: {
    const u16 c = reader.read<u16>();
    result.r = (c & 0xF800) >> 11;
    result.g = (c & 0x07E0) >> 5;
    result.b = (c & 0x001F);
    break;
  }
  case gx::VertexBufferType::Color::rgb8:
    result.r = reader.read<u8>();
    result.g = reader.read<u8>();
    result.b = reader.read<u8>();
    break;
  case gx::VertexBufferType::Color::rgbx8:
    result.r = reader.read<u8>();
    result.g = reader.read<u8>();
    result.b = reader.read<u8>();
    reader.seek(1);
    break;
  case gx::VertexBufferType::Color::rgba4: {
    const u16 c = reader.read<u16>();
    result.r = (c & 0xF000) >> 12;
    result.g = (c & 0x0F00) >> 8;
    result.b = (c & 0x00F0) >> 4;
    result.a = (c & 0x000F);
    break;
  }
  case gx::VertexBufferType::Color::rgba6: {
    const u32 c = reader.read<u32>();
    result.r = (c & 0xFC0000) >> 18;
    result.g = (c & 0x03F000) >> 12;
    result.b = (c & 0x000FC0) >> 6;
    result.a = (c & 0x00003F);
    break;
  }
  case gx::VertexBufferType::Color::rgba8:
    result.r = reader.read<u8>();
    result.g = reader.read<u8>();
    result.b = reader.read<u8>();
    result.a = reader.read<u8>();
    break;
  };

  return result;
}

template <typename T>
inline T readGenericComponents(oishii::BinaryReader& reader,
                               gx::VertexBufferType::Generic type,
                               std::size_t true_count, u32 divisor = 0) {
  assert(true_count <= 3 && true_count >= 1);
  T out{};

  for (std::size_t i = 0; i < true_count; ++i) {
    *(((f32*)&out.x) + i) = readGenericComponentSingle(reader, type, divisor);
  }

  return out;
}

template <typename T>
inline T readComponents(oishii::BinaryReader& reader, gx::VertexBufferType type,
                        std::size_t true_count, u32 divisor = 0) {
  return readGenericComponents<T>(reader, type.generic, true_count, divisor);
}
template <>
inline gx::Color readComponents<gx::Color>(oishii::BinaryReader& reader,
                                           gx::VertexBufferType type,
                                           std::size_t true_count,
                                           u32 divisor) {
  return readColorComponents(reader, type.color);
}

} // namespace gx
} // namespace libcube
