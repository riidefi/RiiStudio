#pragma once

#include <librii/gx/Vertex.hpp>
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

template <typename T, bool HasMinimum, bool HasDivisor,
          librii::gx::VertexBufferKind kind>
struct GenericBuffer {
  std::string mName;
  u32 mId;
  Quantization mQuantize;
  std::vector<T> mEntries;

  std::string getName() const { return mName; }

  bool operator==(const GenericBuffer& rhs) const = default;
};


class PositionBuffer
    : public GenericBuffer<glm::vec3, true, true,
                           librii::gx::VertexBufferKind::position> {
public:
  PositionBuffer(const PositionBuffer&) = default;
  PositionBuffer() {
    mQuantize.mComp = librii::gx::VertexComponentCount(
        librii::gx::VertexComponentCount::Position::xyz);
    mQuantize.mType = librii::gx::VertexBufferType(
        librii::gx::VertexBufferType::Generic::f32);
    mQuantize.stride = 12;
  }
};
class NormalBuffer
    : public GenericBuffer<glm::vec3, false, true,
                           librii::gx::VertexBufferKind::normal> {
public:
  NormalBuffer(const NormalBuffer&) = default;
  NormalBuffer() {
    mQuantize.mComp = librii::gx::VertexComponentCount(
        librii::gx::VertexComponentCount::Normal::xyz);
    mQuantize.mType = librii::gx::VertexBufferType(
        librii::gx::VertexBufferType::Generic::f32);
    mQuantize.stride = 12;
  }
};
class ColorBuffer : public GenericBuffer<librii::gx::Color, false, false,
                                         librii::gx::VertexBufferKind::color> {
public:
  ColorBuffer(const ColorBuffer&) = default;
  ColorBuffer() {
    mQuantize.mComp = librii::gx::VertexComponentCount(
        librii::gx::VertexComponentCount::Color::rgba);
    mQuantize.mType = librii::gx::VertexBufferType(
        librii::gx::VertexBufferType::Color::rgba8);
    mQuantize.stride = 4;
  }
};
class TextureCoordinateBuffer
    : public GenericBuffer<glm::vec2, true, true,
                           librii::gx::VertexBufferKind::textureCoordinate> {
public:
  TextureCoordinateBuffer(const TextureCoordinateBuffer&) = default;
  TextureCoordinateBuffer() {
    mQuantize.mComp = librii::gx::VertexComponentCount(
        librii::gx::VertexComponentCount::TextureCoordinate::uv);
    mQuantize.mType = librii::gx::VertexBufferType(
        librii::gx::VertexBufferType::Generic::f32);
    mQuantize.stride = 8;
  }
};

} // namespace librii::g3d
