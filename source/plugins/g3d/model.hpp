#pragma once

#include <core/3d/i3dmodel.hpp>
#include <core/kpi/Node.hpp>

#include "bone.hpp"
#include "material.hpp"
#include "polygon.hpp"

#include <plugins/gc/gx/VertexTypes.hpp>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace riistudio::g3d {

enum class ScalingRule { Standard, XSI, Maya };
enum class TextureMatrixMode { Maya, XSI, Max };
enum class EnvelopeMatrixMode { Normal, Approximation, Precision };

struct Quantization {
  libcube::gx::VertexComponentCount mComp;
  libcube::gx::VertexBufferType mType;
  u8 divisor;
  u8 stride;

  bool operator==(const Quantization &rhs) const {
    // TODO: Better union comparison
    return mComp.normal == rhs.mComp.normal &&
           mType.generic == rhs.mType.generic && divisor == rhs.divisor &&
           stride == rhs.stride;
  }
};
template <typename T, bool HasMinimum, bool HasDivisor,
          libcube::gx::VertexBufferKind kind>
struct GenericBuffer {
  std::string mName;
  u32 mId;
  std::string getName() const { return mName; }

  Quantization mQuantize;
  std::vector<T> mEntries;

  bool operator==(const GenericBuffer &rhs) const {
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

struct G3DModel : public lib3d::Model {
  virtual ~G3DModel() = default;
  // Shallow comparison
  bool operator==(const G3DModel &rhs) const { return true; }
  const G3DModel &operator=(const G3DModel &rhs) { return *this; }

  ScalingRule mScalingRule;
  TextureMatrixMode mTexMtxMode;
  EnvelopeMatrixMode mEvpMtxMode;
  std::string sourceLocation;
  lib3d::AABB aabb;
};

struct G3DModelAccessor : public kpi::NodeAccessor<G3DModel> {
  KPI_NODE_FOLDER_SIMPLE(Material);
  KPI_NODE_FOLDER_SIMPLE(Bone);

  KPI_NODE_FOLDER_SIMPLE(PositionBuffer);
  KPI_NODE_FOLDER_SIMPLE(NormalBuffer);
  KPI_NODE_FOLDER_SIMPLE(ColorBuffer);
  KPI_NODE_FOLDER_SIMPLE(TextureCoordinateBuffer);

  KPI_NODE_FOLDER_SIMPLE(Polygon);
};

} // namespace riistudio::g3d
