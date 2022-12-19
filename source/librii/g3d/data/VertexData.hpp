#pragma once

#include <librii/gx/Vertex.hpp>
#include <optional>
#include <string>
#include <vector>

//
// TODO: Redo this entire file
//

namespace librii::g3d {

struct Quantization {
  librii::gx::VertexComponentCount mComp;
  librii::gx::VertexBufferType mType;
  u8 divisor = 0;
  u8 stride = 0;

  bool operator==(const Quantization& rhs) const {
    // TODO: Better union comparison
    return mComp.normal == rhs.mComp.normal &&
           mType.generic == rhs.mType.generic && divisor == rhs.divisor &&
           stride == rhs.stride;
  }
};

template <typename T> struct MinMax {
  T min;
  T max;

  bool operator==(const MinMax&) const = default;
};

template <typename T, bool HasMinimum, bool HasDivisor,
          librii::gx::VertexBufferKind kind>
struct GenericBuffer {
  std::string mName;
  u32 mId;
  Quantization mQuantize;
  std::vector<T> mEntries;

  // Min/Max are not re-quantized by official tooling it seems.
  std::optional<MinMax<T>> mCachedMinMax;

  std::string getName() const { return mName; }

  bool operator==(const GenericBuffer& rhs) const = default;
};

template <typename T, bool m, bool d, librii::gx::VertexBufferKind kind>
inline MinMax<T> ComputeMinMax(const GenericBuffer<T, m, d, kind>& buf) {
  MinMax<T> tmp;
  for (int c = 0; c < T::length(); ++c) {
    tmp.min[c] = +1'000'000'000.0f;
    tmp.max[c] = -1'000'000'000.0f;
  }
  for (auto& elem : buf.mEntries) {
    for (int c = 0; c < elem.length(); ++c) {
      tmp.min[c] = std::min(elem[c], tmp.min[c]);
      tmp.max[c] = std::max(elem[c], tmp.max[c]);
    }
  }
  return tmp;
}

namespace Default {

using namespace librii::gx;

static inline Quantization Position = {
    .mComp = VertexComponentCount(VertexComponentCount::Position::xyz),
    .mType = VertexBufferType(VertexBufferType::Generic::f32),
    .stride = 12,
};
static inline Quantization Normal = {
    .mComp = VertexComponentCount(VertexComponentCount::Normal::xyz),
    .mType = VertexBufferType(VertexBufferType::Generic::f32),
    .stride = 12,
};
static inline Quantization Color = {
    .mComp = VertexComponentCount(VertexComponentCount::Color::rgba),
    .mType = VertexBufferType(VertexBufferType::Color::rgba8),
    .stride = 4,
};
static inline Quantization TexCoord = {
    .mComp = VertexComponentCount(VertexComponentCount::TextureCoordinate::uv),
    .mType = VertexBufferType(VertexBufferType::Generic::f32),
    .stride = 8,
};

} // namespace Default

inline std::string ValidateQuantize(librii::gx::VertexBufferKind kind,
                                    const Quantization& quant) {
  if (kind == librii::gx::VertexBufferKind::normal) {
    switch (quant.mType.generic) {
    case librii::gx::VertexBufferType::Generic::s8: {
      if ((int)quant.divisor != 6) {
        return "Invalid divisor for S8 normal data";
      }
      break;
    }
    case librii::gx::VertexBufferType::Generic::s16: {
      if ((int)quant.divisor != 14) {
        return "Invalid divisor for S16 normal data";
      }
      break;
    }
    case librii::gx::VertexBufferType::Generic::u8:
      return "Invalid quantization for normal data: U8";
    case librii::gx::VertexBufferType::Generic::u16:
      return "Invalid quantization for normal data: U16";
    case librii::gx::VertexBufferType::Generic::f32:
      if ((int)quant.divisor != 0) {
        return "Misleading divisor for F32 normal data";
      }
      break;
    }
  }

  return "";
}

class PositionBuffer
    : public GenericBuffer<glm::vec3, true, true,
                           librii::gx::VertexBufferKind::position> {
public:
  PositionBuffer(const PositionBuffer&) = default;
  PositionBuffer() { mQuantize = Default::Position; }
};
class NormalBuffer
    : public GenericBuffer<glm::vec3, false, true,
                           librii::gx::VertexBufferKind::normal> {
public:
  NormalBuffer(const NormalBuffer&) = default;
  NormalBuffer() { mQuantize = Default::Normal; }
};
class ColorBuffer : public GenericBuffer<librii::gx::Color, false, false,
                                         librii::gx::VertexBufferKind::color> {
public:
  ColorBuffer(const ColorBuffer&) = default;
  ColorBuffer() { mQuantize = Default::Color; }
};
class TextureCoordinateBuffer
    : public GenericBuffer<glm::vec2, true, true,
                           librii::gx::VertexBufferKind::textureCoordinate> {
public:
  TextureCoordinateBuffer(const TextureCoordinateBuffer&) = default;
  TextureCoordinateBuffer() { mQuantize = Default::TexCoord; }
};

} // namespace librii::g3d
