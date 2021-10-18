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

class CourseMapData {
public:
  bool operator==(const CourseMapData&) const = default;

  u16 mRevision = 2520;
  u8 mOpeningPanIndex = 0;
  u8 mVideoPanIndex = 0;
};

class CourseMap : public CourseMapData {
public:
  bool operator==(const CourseMap&) const = default;
  bool operator!=(const CourseMap&) const = default;

  rsl::small_vector<StartPoint, 1> mStartPoints;
  rsl::small_vector<EnemyPath, 16> mEnemyPaths;
  rsl::small_vector<ItemPath, 16> mItemPaths;
  rsl::small_vector<CheckPath, 16> mCheckPaths;
  rsl::small_vector<Path, 32> mPaths;
  rsl::small_vector<GeoObj, 64> mGeoObjs;
  rsl::small_vector<Area, 16> mAreas;
  rsl::small_vector<Camera, 16> mCameras;
  rsl::small_vector<RespawnPoint, 32> mRespawnPoints;
  rsl::small_vector<Cannon, 8> mCannonPoints;
  rsl::small_vector<Stage, 1> mStages;
  rsl::small_vector<MissionPoint, 1> mMissionPoints;
};

} // namespace librii::kmp
