#pragma once

#include <core/3d/i3dmodel.hpp>
#include <core/kpi/Node.hpp>

#include "model.hpp"
#include "texture.hpp"

namespace riistudio::g3d {

struct G3DCollection : public lib3d::Scene {
  // Shallow comparison
  bool operator==(const G3DCollection& rhs) const { return true; }
  const G3DCollection& operator=(const G3DCollection& rhs) { return *this; }
};

struct G3DCollectionAccessor : public kpi::NodeAccessor<G3DCollection> {
  using super = kpi::NodeAccessor<G3DCollection>;

  using super::super;

  KPI_NODE_FOLDER(G3DModel, G3DModelAccessor);
  KPI_NODE_FOLDER_SIMPLE(Texture);
};

} // namespace riistudio::g3d
