#pragma once

#include "bone.hpp"
#include "material.hpp"
#include "polygon.hpp"
#include <core/kpi/Node2.hpp>
#include <glm/vec2.hpp> // glm::vec2
#include <glm/vec3.hpp> // glm::vec3
#include <plugins/gc/Export/Scene.hpp>
#include <librii/gx.h>
#include <tuple>

#include "texture.hpp"

namespace riistudio::g3d {

enum class ScalingRule { Standard, XSI, Maya };
enum class TextureMatrixMode { Maya, XSI, Max };
enum class EnvelopeMatrixMode { Normal, Approximation, Precision };

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
struct GenericBuffer : public virtual kpi::IObject {
  std::string mName;
  u32 mId;
  std::string getName() const { return mName; }

  Quantization mQuantize;
  std::vector<T> mEntries;

  bool operator==(const GenericBuffer& rhs) const {
    return mName == rhs.mName && mId == rhs.mId && mQuantize == rhs.mQuantize &&
           mEntries == rhs.mEntries;
  }
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

struct G3DModelData : public virtual kpi::IObject {
  virtual ~G3DModelData() = default;
  // Shallow comparison
  bool operator==(const G3DModelData& rhs) const {
    return mScalingRule == rhs.mScalingRule && mTexMtxMode == rhs.mTexMtxMode &&
           mEvpMtxMode == rhs.mEvpMtxMode &&
           sourceLocation == rhs.sourceLocation && aabb == rhs.aabb;
  }
  const G3DModelData& operator=(const G3DModelData& rhs) { return *this; }

  ScalingRule mScalingRule = ScalingRule::Standard;
  TextureMatrixMode mTexMtxMode = TextureMatrixMode::Maya;
  EnvelopeMatrixMode mEvpMtxMode = EnvelopeMatrixMode::Normal;
  std::string sourceLocation;
  lib3d::AABB aabb;

  std::string mName = "course";

  std::string getName() const { return mName; }
  void setName(const std::string& name) { mName = name; }
};

} // namespace riistudio::g3d

#include "Node.h"

namespace riistudio::g3d {

inline std::pair<u32, u32> computeVertTriCounts(const Polygon& mesh) {
  u32 nVert = 0, nTri = 0;

  // Set of linear equations for computing polygon count from number of
  // vertices. Division by zero acts as multiplication by zero here.
  struct LinEq {
    u32 quot : 4;
    u32 dif : 4;
  };
  constexpr std::array<LinEq, (u32)librii::gx::PrimitiveType::Max> triVertCvt{
      LinEq{4, 0}, // 0 [v / 4] Quads
      LinEq{4, 0}, // 1 [v / 4] QuadStrip
      LinEq{3, 0}, // 2 [v / 3] Triangles
      LinEq{1, 2}, // 3 [v - 2] TriangleStrip
      LinEq{1, 2}, // 4 [v - 2] TriangleFan
      LinEq{0, 0}, // 5 [v * 0] Lines
      LinEq{0, 0}, // 6 [v * 0] LineStrip
      LinEq{0, 0}  // 7 [v * 0] Points
  };

  for (const auto& mprim : mesh.mMatrixPrimitives) {
    for (const auto& prim : mprim.mPrimitives) {
      const u8 pIdx = static_cast<u8>(prim.mType);
      assert(pIdx < static_cast<u8>(librii::gx::PrimitiveType::Max));
      const LinEq& pCvtr = triVertCvt[pIdx];
      const u32 pVCount = prim.mVertices.size();

      if (pVCount == 0)
        continue;

      nVert += pVCount;
      nTri += pCvtr.quot == 0 ? 0 : pVCount / pCvtr.quot - pCvtr.dif;
    }
  }

  return {nVert, nTri};
}
inline std::pair<u32, u32> computeVertTriCounts(const Model& model) {
  u32 nVert = 0, nTri = 0;

  for (const auto& mesh : model.getMeshes()) {
    const auto [vert, tri] = computeVertTriCounts(mesh);
    nVert += vert;
    nTri += tri;
  }

  return {nVert, nTri};
}

} // namespace riistudio::g3d
