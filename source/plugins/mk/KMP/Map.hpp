#pragma once

#include <core/common.h>
#include <plugins/mk/KMP/data/MapArea.hpp>
#include <plugins/mk/KMP/data/MapCamera.hpp>
#include <plugins/mk/KMP/data/MapCannon.hpp>
#include <plugins/mk/KMP/data/MapGeoObj.hpp>
#include <plugins/mk/KMP/data/MapMissionPoint.hpp>
#include <plugins/mk/KMP/data/MapPath.hpp>
#include <plugins/mk/KMP/data/MapRespawn.hpp>
#include <plugins/mk/KMP/data/MapStage.hpp>
#include <plugins/mk/KMP/data/MapStart.hpp>

namespace riistudio::mk {

class CourseMapData {
public:
  bool operator==(const CourseMapData&) const = default;

  u16 getRevision() const { return mRevision; }
  void setRevision(u16 r) { mRevision = r; }

  // Section user data
  u8 getOpeningPanIndex() const { return mOpeningPanIndex; }
  void setOpeningPanIndex(u8 i) { mOpeningPanIndex = i; }
  u8 getVideoPanIndex() const { return mVideoPanIndex; }
  void setVideoPanIndex(u8 i) { mVideoPanIndex = i; }

private:
  u16 mRevision = 2520;
  u8 mOpeningPanIndex = 0;
  u8 mVideoPanIndex = 0;
};

struct NullClass {};

} // namespace riistudio::mk

#include <core/kpi/Node2.hpp>

////////////////////////////////
#include <plugins/mk/KMP/Node.h>
