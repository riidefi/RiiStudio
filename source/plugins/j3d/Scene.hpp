#pragma once

#include <plugins/3d/i3dmodel.hpp>

#include "Joint.hpp"
#include "Material.hpp"
#include "Shape.hpp"
#include "Texture.hpp"
#include "VertexBuffer.hpp"
#include <plugins/gc/Export/Scene.hpp>

#include <librii/j3d/J3dIo.hpp>

namespace riistudio::j3d {

using Bufs = librii::j3d::Bufs;

struct ModelData_ {
  librii::j3d::ScalingRule mScalingRule = librii::j3d::ScalingRule::Basic;
  bool isBDL = false;
  Bufs mBufs;

  bool operator==(const ModelData_&) const = default;
};

struct ModelData : public virtual kpi::IObject, public ModelData_ {
  virtual ~ModelData() = default;
  // Shallow comparison
  bool operator==(const ModelData& rhs) const {
    return static_cast<const ModelData_&>(*this) == rhs;
  }

  std::string getName() const { return "Model"; }
};

} // namespace riistudio::j3d

#include "Node.h"
