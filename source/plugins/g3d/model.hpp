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
#include <librii/g3d/data/VertexData.hpp>
#include <librii/gx.h>
#include <plugins/gc/Export/Scene.hpp>
#include <tuple>

namespace riistudio::g3d {


struct PositionBuffer : public librii::g3d::PositionBuffer,
                        public virtual kpi::IObject {
  using Parent = librii::g3d::PositionBuffer;
  std::string getName() const {
    return static_cast<const Parent&>(*this).mName;
  }

  bool operator==(const PositionBuffer& rhs) const {
    return static_cast<const Parent&>(*this) == rhs;
  }
};
struct NormalBuffer : public librii::g3d::NormalBuffer,
                        public virtual kpi::IObject {
  using Parent = librii::g3d::NormalBuffer;
  std::string getName() const {
    return static_cast<const Parent&>(*this).mName;
  }

  bool operator==(const NormalBuffer& rhs) const {
    return static_cast<const Parent&>(*this) == rhs;
  }
};
struct ColorBuffer : public librii::g3d::ColorBuffer,
                        public virtual kpi::IObject {
  using Parent = librii::g3d::ColorBuffer;
  std::string getName() const {
    return static_cast<const Parent&>(*this).mName;
  }

  bool operator==(const ColorBuffer& rhs) const {
    return static_cast<const Parent&>(*this) == rhs;
  }
};
struct TextureCoordinateBuffer : public librii::g3d::TextureCoordinateBuffer,
                     public virtual kpi::IObject {
  using Parent = librii::g3d::TextureCoordinateBuffer;
  std::string getName() const {
    return static_cast<const Parent&>(*this).mName;
  }

  bool operator==(const TextureCoordinateBuffer& rhs) const {
    return static_cast<const Parent&>(*this) == rhs;
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
