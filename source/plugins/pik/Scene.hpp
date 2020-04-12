#pragma once

#include <core/3d/i3dmodel.hpp>
#include <core/kpi/Node.hpp>

namespace riistudio::pik {

struct PikminCollection : public lib3d::Scene {
  // Shallow comparison
  bool operator==(const PikminCollection &rhs) const { return true; }
  const PikminCollection &operator=(const PikminCollection &rhs) {
    return *this;
  }
};

struct PikminModel : public lib3d::Model {
  virtual ~PikminModel() = default;
  // Shallow comparison
  bool operator==(const PikminModel &rhs) const {
    return nJoints == rhs.nJoints;
  }
  const PikminModel &operator=(const PikminModel &rhs) { return *this; }

  std::size_t nJoints = 0;
};

struct PikminModelAccessor : public kpi::NodeAccessor<PikminModel> {
  //	KPI_NODE_FOLDER_SIMPLE(Material);
  //	KPI_NODE_FOLDER_SIMPLE(Bone);
  //
  //	KPI_NODE_FOLDER_SIMPLE(PositionBuffer);
  //	KPI_NODE_FOLDER_SIMPLE(NormalBuffer);
  //	KPI_NODE_FOLDER_SIMPLE(ColorBuffer);
  //	KPI_NODE_FOLDER_SIMPLE(TextureCoordinateBuffer);
  //
  //	KPI_NODE_FOLDER_SIMPLE(Polygon);
};

struct PikminCollectionAccessor : public kpi::NodeAccessor<PikminCollection> {
  using super = kpi::NodeAccessor<PikminCollection>;
  using super::super;

  KPI_NODE_FOLDER(PikminModel, PikminModelAccessor);
};

} // namespace riistudio::pik
