#pragma once

#include <core/common.h>
#include <librii/kmp/data/MapArea.hpp>
#include <librii/kmp/data/MapCamera.hpp>
#include <librii/kmp/data/MapCannon.hpp>
#include <librii/kmp/data/MapGeoObj.hpp>
#include <librii/kmp/data/MapMissionPoint.hpp>
#include <librii/kmp/data/MapPath.hpp>
#include <librii/kmp/data/MapRespawn.hpp>
#include <librii/kmp/data/MapStage.hpp>
#include <librii/kmp/data/MapStart.hpp>
#include <rsl/SmallVector.hpp>

namespace librii::kmp {

struct CourseMap {
  u16 mRevision = 2520;
  u8 mOpeningPanIndex = 0;
  u8 mVideoPanIndex = 0;
  std::vector<StartPoint> mStartPoints;
  std::vector<EnemyPath> mEnemyPaths;
  std::vector<ItemPath> mItemPaths;
  std::vector<CheckPath> mCheckPaths;
  std::vector<Path> mPaths;
  std::vector<GeoObj> mGeoObjs;
  std::vector<Area> mAreas;
  std::vector<Camera> mCameras;
  std::vector<RespawnPoint> mRespawnPoints;
  std::vector<Cannon> mCannonPoints;
  std::vector<Stage> mStages;
  std::vector<MissionPoint> mMissionPoints;

  bool operator==(const CourseMap&) const = default;
};

std::string DumpJSON(const CourseMap& map);
CourseMap LoadJSON(std::string_view map);

} // namespace librii::kmp
