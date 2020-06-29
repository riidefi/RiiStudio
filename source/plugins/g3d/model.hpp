#pragma once

#include "bone.hpp"
#include "material.hpp"
#include "polygon.hpp"
#include <core/kpi/Node2.hpp>
#include <glm/vec2.hpp> // glm::vec2
#include <glm/vec3.hpp> // glm::vec3
#include <plugins/gc/Export/Scene.hpp>
#include <plugins/gc/GX/VertexTypes.hpp>

#include "texture.hpp"

namespace riistudio::g3d {

enum class ScalingRule { Standard, XSI, Maya };
enum class TextureMatrixMode { Maya, XSI, Max };
enum class EnvelopeMatrixMode { Normal, Approximation, Precision };

struct Quantization {
  libcube::gx::VertexComponentCount mComp;
  libcube::gx::VertexBufferType mType;
  u8 divisor;
  u8 stride;

  bool operator==(const Quantization& rhs) const {
    // TODO: Better union comparison
    return mComp.normal == rhs.mComp.normal &&
           mType.generic == rhs.mType.generic && divisor == rhs.divisor &&
           stride == rhs.stride;
  }
};
template <typename T, bool HasMinimum, bool HasDivisor,
          libcube::gx::VertexBufferKind kind>
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
                           libcube::gx::VertexBufferKind::position> {};
class NormalBuffer
    : public GenericBuffer<glm::vec3, false, true,
                           libcube::gx::VertexBufferKind::normal> {};
class ColorBuffer : public GenericBuffer<libcube::gx::Color, false, false,
                                         libcube::gx::VertexBufferKind::color> {
};
class TextureCoordinateBuffer
    : public GenericBuffer<glm::vec2, true, true,
                           libcube::gx::VertexBufferKind::textureCoordinate> {};

struct G3DModelData {
  virtual ~G3DModelData() = default;
  // Shallow comparison
  bool operator==(const G3DModelData& rhs) const {
    return mScalingRule == rhs.mScalingRule && mTexMtxMode == rhs.mTexMtxMode &&
           mEvpMtxMode == rhs.mEvpMtxMode &&
           sourceLocation == rhs.sourceLocation && aabb == rhs.aabb;
  }
  const G3DModelData& operator=(const G3DModelData& rhs) { return *this; }

  ScalingRule mScalingRule;
  TextureMatrixMode mTexMtxMode;
  EnvelopeMatrixMode mEvpMtxMode;
  std::string sourceLocation;
  lib3d::AABB aabb;

  std::string mName;
  void setName(const std::string& name) { mName = name; }
};

} // namespace riistudio::g3d

#include "Node.h"
