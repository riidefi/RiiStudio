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
#include <llvm/ADT/SmallVector.h>

namespace librii::kmp {

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

class CourseMap : public CourseMapData {
public:
  bool operator==(const CourseMap&) const = default;
  bool operator!=(const CourseMap&) const = default;

  llvm::SmallVector<StartPoint, 1> mStartPoints;
  llvm::SmallVector<EnemyPath, 16> mEnemyPaths;
  llvm::SmallVector<ItemPath, 16> mItemPaths;
  llvm::SmallVector<CheckPath, 16> mCheckPaths;
  llvm::SmallVector<Path, 32> mPaths;
  llvm::SmallVector<GeoObj, 64> mGeoObjs;
  llvm::SmallVector<Area, 16> mAreas;
  llvm::SmallVector<Camera, 16> mCameras;
  llvm::SmallVector<RespawnPoint, 32> mRespawnPoints;
  llvm::SmallVector<Cannon, 8> mCannonPoints;
  llvm::SmallVector<Stage, 1> mStages;
  llvm::SmallVector<MissionPoint, 1> mMissionPoints;
};

} // namespace librii::kmp
