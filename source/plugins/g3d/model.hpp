#pragma once

#include "bone.hpp"
#include "material.hpp"
#include "polygon.hpp"
#include "texture.hpp"
#include <core/kpi/Node2.hpp>
#include <glm/vec2.hpp> // glm::vec2
#include <glm/vec3.hpp> // glm::vec3
#include <librii/g3d/data/AnimData.hpp>
#include <librii/g3d/data/ModelData.hpp>
#include <librii/gx.h>
#include <plugins/gc/Export/Scene.hpp>
#include <tuple>

namespace riistudio::g3d {

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

struct G3DModelData : public librii::g3d::G3DModelDataData,
                      public virtual kpi::IObject {
  virtual ~G3DModelData() = default;
  // Shallow comparison
  bool operator==(const G3DModelData& rhs) const {
    return static_cast<const G3DModelDataData&>(rhs) == *this;
  }
  const G3DModelData& operator=(const G3DModelData& rhs) {
    static_cast<G3DModelDataData&>(*this) = rhs;
    return *this;
  }

  std::string getName() const { return mName; }
  void setName(const std::string& name) { mName = name; }
};

struct SRT0 : public librii::g3d::SrtAnimationArchive,
              public virtual kpi::IObject {
  bool operator==(const SRT0& rhs) const {
    return static_cast<const librii::g3d::SrtAnimationArchive&>(*this) == rhs;
  }
  SRT0& operator=(const SRT0& rhs) {
    static_cast<librii::g3d::SrtAnimationArchive&>(*this) = rhs;
    return *this;
  }

  std::string getName() const { return name; }
  void setName(const std::string& _name) { name = _name; }
};

} // namespace riistudio::g3d

#include "Node.h"

namespace riistudio::g3d {

inline std::pair<u32, u32> computeVertTriCounts(const Model& model) {
  u32 nVert = 0, nTri = 0;

  for (const auto& mesh : model.getMeshes()) {
    const auto [vert, tri] = librii::gx::ComputeVertTriCounts(mesh);
    nVert += vert;
    nTri += tri;
  }

  return {nVert, nTri};
}

} // namespace riistudio::g3d
