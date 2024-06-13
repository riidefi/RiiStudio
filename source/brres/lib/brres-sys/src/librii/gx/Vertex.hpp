#pragma once

#include <core/common.h>
#include <oishii/reader/binary_reader.hxx>
#include <oishii/writer/binary_writer.hxx>
#include <vendor/glm/vec3.hpp>

namespace librii::gx {

namespace VCC {
enum class Position { xy, xyz };
enum class Normal {
  xyz,
  nbt, // index NBT triplets
  nbt3 // index N/B/T individually
};
enum class Color { rgb, rgba };
enum class TextureCoordinate {
  s,
  st,

  u = s,
  uv = st
};

template <typename T> static constexpr T Default();
template <> constexpr Position Default<Position>() { return Position::xyz; }
template <> constexpr Normal Default<Normal>() { return Normal::xyz; }
template <> constexpr Color Default<Color>() { return Color::rgba; }
template <> constexpr TextureCoordinate Default<TextureCoordinate>() {
  return TextureCoordinate::st;
}
}; // namespace VCC

struct VertexComponentCount {
  using Position = VCC::Position;
  using Normal = VCC::Normal;
  using Color = VCC::Color;
  using TextureCoordinate = VCC::TextureCoordinate;

  Position position{VCC::Default<Position>()};
  Normal normal{VCC::Default<Normal>()};
  Color color{VCC::Default<Color>()};
  TextureCoordinate texcoord{VCC::Default<TextureCoordinate>()};

  explicit VertexComponentCount(Position p) : position(p) {}
  explicit VertexComponentCount(Normal n) : normal(n) {}
  explicit VertexComponentCount(Color c) : color(c) {}
  explicit VertexComponentCount(TextureCoordinate u) : texcoord(u) {}
  VertexComponentCount() {}

  bool operator==(const VertexComponentCount&) const = default;
};

struct VertexBufferType {
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
  bool operator==(const VertexBufferType&) const = default;
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
  PrimType, // TODO: Hack for renderer

  Undefined = 0xff - 1,
  Terminate = 0xff,
};
inline VertexAttribute ColorN(u32 n) {
  assert(n < 2);
  return static_cast<VertexAttribute>(
      static_cast<int>(VertexAttribute::Color0) + n);
}
inline VertexAttribute TexCoordN(u32 n) {
  assert(n < 8);
  return static_cast<VertexAttribute>(
      static_cast<int>(VertexAttribute::TexCoord0) + n);
}
constexpr bool IsTexNMtxIdx(VertexAttribute a) {
  return (int)a >= (int)VertexAttribute::Texture0MatrixIndex &&
         (int)a <= (int)VertexAttribute::Texture7MatrixIndex;
}
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

inline Result<std::size_t>
computeComponentCount(gx::VertexBufferKind kind,
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
  }
  EXPECT(false, "Invalid component count!");
}

inline Result<f32>
readGenericComponentSingle(oishii::BinaryReader& reader,
                           gx::VertexBufferType::Generic type,
                           u32 divisor = 0) {
  switch (type) {
  case gx::VertexBufferType::Generic::u8:
    return static_cast<f32>(TRY(reader.tryRead<u8>())) /
           static_cast<f32>((1 << divisor));
  case gx::VertexBufferType::Generic::s8:
    return static_cast<f32>(TRY(reader.tryRead<s8>())) /
           static_cast<f32>((1 << divisor));
  case gx::VertexBufferType::Generic::u16:
    return static_cast<f32>(TRY(reader.tryRead<u16>())) /
           static_cast<f32>((1 << divisor));
  case gx::VertexBufferType::Generic::s16:
    return static_cast<f32>(TRY(reader.tryRead<s16>())) /
           static_cast<f32>((1 << divisor));
  case gx::VertexBufferType::Generic::f32:
    return TRY(reader.tryRead<f32>());
  }
  EXPECT(false, "Invalid VertexBufferType::Generic");
}
inline Result<gx::Color> readColorComponents(oishii::BinaryReader& reader,
                                             gx::VertexBufferType::Color type) {
  gx::Color result; // TODO: Default color

  switch (type) {
  case gx::VertexBufferType::Color::rgb565: {
    const u16 c = TRY(reader.tryRead<u16>());
    result.r = static_cast<float>((c & 0xF800) >> 11) * (255.0f / 31.0f);
    result.g = static_cast<float>((c & 0x07E0) >> 5) * (255.0f / 63.0f);
    result.b = static_cast<float>(c & 0x001F) * (255.0f / 31.0f);
    return result;
  }
  case gx::VertexBufferType::Color::rgb8:
    result.r = TRY(reader.tryRead<u8>());
    result.g = TRY(reader.tryRead<u8>());
    result.b = TRY(reader.tryRead<u8>());
    return result;
  case gx::VertexBufferType::Color::rgbx8:
    result.r = TRY(reader.tryRead<u8>());
    result.g = TRY(reader.tryRead<u8>());
    result.b = TRY(reader.tryRead<u8>());
    reader.skip(1);
    return result;
  case gx::VertexBufferType::Color::rgba4: {
    const u16 c = TRY(reader.tryRead<u16>());
    result.r = ((c & 0xF000) >> 12) * 17;
    result.g = ((c & 0x0F00) >> 8) * 17;
    result.b = ((c & 0x00F0) >> 4) * 17;
    result.a = (c & 0x000F) * 17;
    return result;
  }
  case gx::VertexBufferType::Color::rgba6: {
    u32 c = 0;
    c |= TRY(reader.tryRead<u8>()) << 16;
    c |= TRY(reader.tryRead<u8>()) << 8;
    c |= TRY(reader.tryRead<u8>());
    result.r = static_cast<float>((c & 0xFC0000) >> 18) * (255.0f / 63.0f);
    result.g = static_cast<float>((c & 0x03F000) >> 12) * (255.0f / 63.0f);
    result.b = static_cast<float>((c & 0x000FC0) >> 6) * (255.0f / 63.0f);
    result.a = static_cast<float>(c & 0x00003F) * (255.0f / 63.0f);
    return result;
  }
  case gx::VertexBufferType::Color::rgba8:
    result.r = TRY(reader.tryRead<u8>());
    result.g = TRY(reader.tryRead<u8>());
    result.b = TRY(reader.tryRead<u8>());
    result.a = TRY(reader.tryRead<u8>());
    return result;
  };
  EXPECT(false, "Invalid VertexBufferType::Color");
}

template <typename T>
inline Result<T> readGenericComponents(oishii::BinaryReader& reader,
                                       gx::VertexBufferType::Generic type,
                                       std::size_t true_count,
                                       u32 divisor = 0) {
  EXPECT(true_count <= 3 && true_count >= 1);
  T out{};

  for (std::size_t i = 0; i < true_count; ++i) {
    ((f32*)&out.x)[i] = TRY(readGenericComponentSingle(reader, type, divisor));
  }

  return out;
}

template <typename T>
inline Result<T> readComponents(oishii::BinaryReader& reader,
                                gx::VertexBufferType type,
                                std::size_t true_count, u32 divisor = 0) {
  return readGenericComponents<T>(reader, type.generic, true_count, divisor);
}
template <>
inline Result<gx::Color> readComponents<gx::Color>(oishii::BinaryReader& reader,
                                                   gx::VertexBufferType type,
                                                   std::size_t true_count,
                                                   u32 divisor) {
  return readColorComponents(reader, type.color);
}

[[nodiscard]] inline Result<void>
writeColorComponents(oishii::Writer& writer, const librii::gx::Color& c,
                     VertexBufferType::Color colort) {
  switch (colort) {
  case librii::gx::VertexBufferType::Color::rgb565:
    writer.write<u16>(((c.r & 0xf8) << 8) | ((c.g & 0xfc) << 3) |
                      ((c.b & 0xf8) >> 3));
    break;
  case librii::gx::VertexBufferType::Color::rgb8:
    writer.write<u8>(c.r);
    writer.write<u8>(c.g);
    writer.write<u8>(c.b);
    break;
  case librii::gx::VertexBufferType::Color::rgbx8:
    writer.write<u8>(c.r);
    writer.write<u8>(c.g);
    writer.write<u8>(c.b);
    writer.write<u8>(0);
    break;
  case librii::gx::VertexBufferType::Color::rgba4:
    writer.write<u16>(((c.r & 0xf0) << 8) | ((c.g & 0xf0) << 4) | (c.b & 0xf0) |
                      ((c.a & 0xf0) >> 4));
    break;
  case librii::gx::VertexBufferType::Color::rgba6: {
    const u32 v = ((c.r & 0xfc) << 16) | ((c.g & 0xfc) << 10) |
                  ((c.b & 0xfc) << 4) | ((c.a & 0xfc) >> 2);
    writer.write<u8>((v & 0x00ff0000) >> 16);
    writer.write<u8>((v & 0x0000ff00) >> 8);
    writer.write<u8>((v & 0x000000ff));
    break;
  }
  case librii::gx::VertexBufferType::Color::rgba8:
    writer.write<u8>(c.r);
    writer.write<u8>(c.g);
    writer.write<u8>(c.b);
    writer.write<u8>(c.a);
    break;
  default:
    EXPECT(false, "Invalid buffer type!");
    break;
  }
  return {};
}

template <int n, typename T, glm::qualifier q>
[[nodiscard]] inline Result<void>
writeGenericComponents(oishii::Writer& writer, const glm::vec<n, T, q>& v,
                       librii::gx::VertexBufferType::Generic g,
                       u32 real_component_count, u32 divisor) {
  for (u32 i = 0; i < real_component_count; ++i) {
    switch (g) {
    case librii::gx::VertexBufferType::Generic::u8:
      writer.write<u8>(roundf(v[i] * (1 << divisor)));
      break;
    case librii::gx::VertexBufferType::Generic::s8:
      writer.write<s8>(roundf(v[i] * (1 << divisor)));
      break;
    case librii::gx::VertexBufferType::Generic::u16:
      writer.write<u16>(roundf(v[i] * (1 << divisor)));
      break;
    case librii::gx::VertexBufferType::Generic::s16:
      writer.write<s16>(roundf(v[i] * (1 << divisor)));
      break;
    case librii::gx::VertexBufferType::Generic::f32:
      writer.write<f32>(v[i]);
      break;
    default:
      EXPECT(false, "Invalid buffer type!");
      break;
    }
  }
  return {};
}
template <typename T>
inline Result<void> writeComponents(oishii::Writer& writer, const T& d,
                                    gx::VertexBufferType type,
                                    std::size_t true_count, u32 divisor = 0) {
  return writeGenericComponents(writer, d, type.generic, true_count, divisor);
}
template <>
inline Result<void>
writeComponents<gx::Color>(oishii::Writer& writer, const gx::Color& c,
                           gx::VertexBufferType type, std::size_t true_count,
                           u32 divisor) {
  return writeColorComponents(writer, c, type.color);
}

struct VQuantization {
  librii::gx::VertexComponentCount comp = librii::gx::VertexComponentCount(
      librii::gx::VertexComponentCount::Position::xyz);
  librii::gx::VertexBufferType type =
      librii::gx::VertexBufferType(librii::gx::VertexBufferType::Generic::f32);
  u8 divisor = 0;
  u8 bad_divisor = 0; //!< Accommodation for a bug on N's part
  u8 stride = 12;

  bool operator==(const VQuantization&) const = default;
};
using VBufferKind = VertexBufferKind;

template <typename TB, VBufferKind kind> struct VertexBuffer {
  VQuantization mQuant;
  std::vector<TB> mData;

  Result<std::size_t> ComputeComponentCount() const {
    return computeComponentCount(kind, mQuant.comp);
  }

  template <int n, typename T, glm::qualifier q>
  Result<void> readBufferEntryGeneric(oishii::BinaryReader& reader,
                                      glm::vec<n, T, q>& result) {
    result = TRY(readGenericComponents<glm::vec<n, T, q>>(
        reader, mQuant.type.generic, TRY(ComputeComponentCount()),
        mQuant.divisor));
    return {};
  }
  Result<void> readBufferEntryColor(oishii::BinaryReader& reader,
                                    librii::gx::Color& result) {
    result = TRY(readColorComponents(reader, mQuant.type.color));
    return {};
  }

  template <int n, typename T, glm::qualifier q>
  [[nodiscard]] Result<void>
  writeBufferEntryGeneric(oishii::Writer& writer,
                          const glm::vec<n, T, q>& v) const {
    auto count = ComputeComponentCount();
    EXPECT(count.has_value() && "ComputeComponentCount()");
    return writeGenericComponents(writer, v, mQuant.type.generic, *count,
                                  mQuant.divisor);
  }
  [[nodiscard]] Result<void>
  writeBufferEntryColor(oishii::Writer& writer,
                        const librii::gx::Color& c) const {
    return writeColorComponents(writer, c, mQuant.type.color);
  }
  // For dead cases
  template <int n, typename T, glm::qualifier q>
  [[nodiscard]] Result<void>
  writeBufferEntryColor(oishii::Writer& writer,
                        const glm::vec<n, T, q>& v) const {
    EXPECT(false, "Invalid kind/type template match!");
  }
  [[nodiscard]] Result<void>
  writeBufferEntryGeneric(oishii::Writer& writer,
                          const librii::gx::Color& c) const {
    EXPECT(false, "Invalid kind/type template match!");
  }

  [[nodiscard]] Result<void> writeData(oishii::Writer& writer) const {
    if constexpr (kind == VBufferKind::position) {
      if (mQuant.comp.position ==
          librii::gx::VertexComponentCount::Position::xy) {
        EXPECT(false, "Buffer: XY Pos Component count.");
      }

      for (const auto& d : mData) {
        TRY(writeBufferEntryGeneric(writer, d));
      }
    } else if constexpr (kind == VBufferKind::normal) {
      if (mQuant.comp.normal != librii::gx::VertexComponentCount::Normal::xyz) {
        EXPECT(false, "Buffer: NBT Vectors.");
      }

      for (const auto& d : mData) {
        TRY(writeBufferEntryGeneric(writer, d));
      }
    } else if constexpr (kind == VBufferKind::color) {
      for (const auto& d : mData) {
        TRY(writeBufferEntryColor(writer, d));
      }
    } else if constexpr (kind == VBufferKind::textureCoordinate) {
      if (mQuant.comp.texcoord ==
          librii::gx::VertexComponentCount::TextureCoordinate::s) {
        EXPECT(false, "Buffer: Single component texcoord vectors.");
      }

      for (const auto& d : mData) {
        TRY(writeBufferEntryGeneric(writer, d));
      }
    }

    return {};
  }

  VertexBuffer() {}
  VertexBuffer(VQuantization q) : mQuant(q) {}
  bool operator==(const VertexBuffer&) const = default;
};

} // namespace librii::gx
