#include "CourseMap.hpp"
#include <fmt/color.h>
#include <librii/objflow/ObjFlow.hpp>
#include <vendor/nlohmann/json.hpp>

using namespace librii::kmp;
using json = nlohmann::json;

namespace glm {
void to_json(json& j, const vec3& p) { j = json{p.x, p.y, p.z}; }

void from_json(const json& j, vec3& p) {
  j.at(0).get_to(p.x);
  j.at(1).get_to(p.y);
  j.at(2).get_to(p.z);
}
void to_json(json& j, const vec2& p) { j = json{p.x, p.y}; }

void from_json(const json& j, vec2& p) {
  j.at(0).get_to(p.x);
  j.at(1).get_to(p.y);
}
} // namespace glm

namespace librii::kmp {

const auto Flow = *objflow::Default();

Result<std::string> IdToStr(int id) {
	if (id == 0) {
    return "InvalidID:0x00";
  }
  if (id >= Flow.remap_table.size() || id < 0 ||
      Flow.remap_table[id] >= Flow.parameters.size() ||
      Flow.remap_table[id] < 0) {
    return std::unexpected(
        std::format("Cannot find name corresponding to ID {}", id));
  }
  return Flow.parameters[Flow.remap_table[id]].Name;
}
Result<int> StrToId(std::string_view name) {
  for (int i = 0; i < Flow.remap_table.size(); ++i) {
    if (Flow.parameters[Flow.remap_table[i]].Name == name) {
      return i;
    }
  }

  return std::unexpected(
      std::format("Cannot find ID corresponding to name {}", name));
}

NLOHMANN_JSON_SERIALIZE_ENUM(AreaShape, {
                                            {AreaShape::Box, "Box"},
                                            {AreaShape::Cylinder, "Cylinder"},
                                        })

NLOHMANN_JSON_SERIALIZE_ENUM(
    AreaType, {
                  {AreaType::Camera, "Camera"},
                  {AreaType::EffectController, "EffectController"},
                  {AreaType::FogController, "FogController"},
                  {AreaType::PullController, "PullController"},
                  {AreaType::EnemyFall, "EnemyFall"},
                  {AreaType::MapArea2D, "MapArea2D"},
                  {AreaType::BloomController, "BloomController"},
                  {AreaType::TeresaController, "TeresaController"},
                  {AreaType::ObjClipClassifier, "ObjClipClassifier"},
                  {AreaType::ObjClipDiscriminator, "ObjClipDiscriminator"},
                  {AreaType::PlayerBoundary, "PlayerBoundary"},
              })

void to_json(json& j, const AreaModel& am) {
  j = json{{"mShape", am.mShape},
           {"mPosition", am.mPosition},
           {"mRotation", am.mRotation},
           {"mScaling", am.mScaling}};
}

void from_json(const json& j, AreaModel& am) {
  j.at("mShape").get_to(am.mShape);
  j.at("mPosition").get_to(am.mPosition);
  j.at("mRotation").get_to(am.mRotation);
  j.at("mScaling").get_to(am.mScaling);
}

void to_json(json& j, const Area& a) {
  j = json{{"mType", static_cast<int>(a.mType)}, {"mModel", a.mModel},
           {"mCameraIndex", a.mCameraIndex},     {"mPriority", a.mPriority},
           {"mParameters", a.mParameters},       {"mRailID", a.mRailID},
           {"mEnemyLinkID", a.mEnemyLinkID},     {"mPad", a.mPad}};
}

void from_json(const json& j, Area& a) {
  j.at("mType").get_to(a.mType);
  from_json(j["mModel"], a.mModel);
  j.at("mCameraIndex").get_to(a.mCameraIndex);
  j.at("mPriority").get_to(a.mPriority);
  j.at("mParameters").get_to(a.mParameters);
  j.at("mRailID").get_to(a.mRailID);
  j.at("mEnemyLinkID").get_to(a.mEnemyLinkID);
  j.at("mPad").get_to(a.mPad);
}

NLOHMANN_JSON_SERIALIZE_ENUM(CameraType,
                             {
                                 {CameraType::Goal, "Goal"},
                                 {CameraType::Fixed, "Fixed"},
                                 {CameraType::Path, "Path"},
                                 {CameraType::Follow, "Follow"},
                                 {CameraType::FixedMoveAt, "FixedMoveAt"},
                                 {CameraType::PathMoveAt, "PathMoveAt"},
                                 {CameraType::FollowPath, "FollowPath"},
                                 {CameraType::FollowPath2, "FollowPath2"},
                                 {CameraType::FollowPath3, "FollowPath3"},
                                 {CameraType::MissionSuccess, "MissionSuccess"},
                             })

void to_json(json& j, const LinearAttribute<float>& la) {
  j = json{{"mSpeed", la.mSpeed}, {"from", la.from}, {"to", la.to}};
}

void from_json(const json& j, LinearAttribute<float>& la) {
  j.at("mSpeed").get_to(la.mSpeed);
  j.at("from").get_to(la.from);
  j.at("to").get_to(la.to);
}
void to_json(json& j, const LinearAttribute<glm::vec3>& la) {
  j = json{{"mSpeed", la.mSpeed}, {"from", la.from}, {"to", la.to}};
}

void from_json(const json& j, LinearAttribute<glm::vec3>& la) {
  j.at("mSpeed").get_to(la.mSpeed);
  j.at("from").get_to(la.from);
  j.at("to").get_to(la.to);
}

void to_json(json& j, const Camera& c) {
  j = json{{"mType", c.mType},           {"mNext", c.mNext},
           {"mShake", c.mShake},         {"mPathId", c.mPathId},
           {"mPathSpeed", c.mPathSpeed}, {"mStartFlag", c.mStartFlag},
           {"mMovieFlag", c.mMovieFlag}, {"mPosition", c.mPosition},
           {"mRotation", c.mRotation},   {"mFov", c.mFov},
           {"mView", c.mView},           {"mActiveFrames", c.mActiveFrames}};
}

void from_json(const json& j, Camera& c) {
  j.at("mType").get_to(c.mType);
  j.at("mNext").get_to(c.mNext);
  j.at("mShake").get_to(c.mShake);
  j.at("mPathId").get_to(c.mPathId);
  j.at("mPathSpeed").get_to(c.mPathSpeed);
  j.at("mStartFlag").get_to(c.mStartFlag);
  j.at("mMovieFlag").get_to(c.mMovieFlag);
  j.at("mPosition").get_to(c.mPosition);
  j.at("mRotation").get_to(c.mRotation);
  j.at("mFov").get_to(c.mFov);
  j.at("mView").get_to(c.mView);
  j.at("mActiveFrames").get_to(c.mActiveFrames);
}

NLOHMANN_JSON_SERIALIZE_ENUM(CannonType, {
                                             {CannonType::Direct, "Direct"},
                                             {CannonType::Curved, "Curved"},
                                             {CannonType::Curved2, "Curved2"},
                                         });

void to_json(json& j, const Cannon& c) {
  j = json{{"mType", c.mType},
           {"mPosition", c.mPosition},
           {"mRotation", c.mRotation}};
}

void from_json(const json& j, Cannon& c) {
  j.at("mType").get_to(c.mType);
  j.at("mPosition").get_to(c.mPosition);
  j.at("mRotation").get_to(c.mRotation);
}
void to_json(json& j, const GeoObj& obj) {
  auto str = IdToStr(obj.id);
  if (!str) {
    fmt::print(stderr, fmt::fg(fmt::color::dark_red), "KMP->JSON: {}",
               str.error());
    abort();
  }
  j = json{{"id", *str},
           {"_", obj._},
           {"position", obj.position},
           {"rotation", obj.rotation},
           {"scale", obj.scale},
           {"pathId", obj.pathId},
           {"settings", obj.settings},
           {"flags", obj.flags}};
}

void from_json(const json& j, GeoObj& obj) {
  std::string s = j["id"];
  auto id = StrToId(s);
  if (!id) {
    fmt::print(stderr, fmt::fg(fmt::color::dark_red), "JSON->KMP: {}",
               id.error());
    abort();
  }
  obj.id = *id;
  j.at("_").get_to(obj._);
  j.at("position").get_to(obj.position);
  j.at("rotation").get_to(obj.rotation);
  j.at("scale").get_to(obj.scale);
  j.at("pathId").get_to(obj.pathId);
  j.at("settings").get_to(obj.settings);
  j.at("flags").get_to(obj.flags);
}
void to_json(json& j, const MissionPoint& point) {
  j = json{{"position", point.position},
           {"rotation", point.rotation},
           {"id", point.id},
           {"unknown", point.unknown}};
}

void from_json(const json& j, MissionPoint& point) {
  j.at("position").get_to(point.position);
  j.at("rotation").get_to(point.rotation);
  j.at("id").get_to(point.id);
  j.at("unknown").get_to(point.unknown);
}

void to_json(json& j, const CheckPoint& point) {
  j = json{{"mLeft", point.mLeft},
           {"mRight", point.mRight},
           {"mRespawnIndex", point.mRespawnIndex},
           {"mLapCheck", point.mLapCheck}};
}

void from_json(const json& j, CheckPoint& point) {
  j.at("mLeft").get_to(point.mLeft);
  j.at("mRight").get_to(point.mRight);
  j.at("mRespawnIndex").get_to(point.mRespawnIndex);
  j.at("mLapCheck").get_to(point.mLapCheck);
}

void to_json(nlohmann::json& j, const EnemyPoint& e) {
  j = nlohmann::json{
      {"position", e.position}, {"deviation", e.deviation}, {"param", e.param}};
}

void from_json(const nlohmann::json& j, EnemyPoint& e) {
  j.at("position").get_to(e.position);
  j.at("deviation").get_to(e.deviation);
  j.at("param").get_to(e.param);
}

void to_json(nlohmann::json& j, const ItemPoint& i) {
  j = nlohmann::json{
      {"position", i.position}, {"deviation", i.deviation}, {"param", i.param}};
}

void from_json(const nlohmann::json& j, ItemPoint& i) {
  j.at("position").get_to(i.position);
  j.at("deviation").get_to(i.deviation);
  j.at("param").get_to(i.param);
}
template <typename PointT>
void to_json(json& j, const DirectedGraph<PointT>& graph) {
  j = json{{"points", graph.points},
           {"mPredecessors", graph.mPredecessors},
           {"mSuccessors", graph.mSuccessors},
           {"misc", graph.misc}};
}

template <typename PointT>
void from_json(const json& j, DirectedGraph<PointT>& graph) {
  j.at("points").get_to(graph.points);
  j.at("mPredecessors").get_to(graph.mPredecessors);
  j.at("mSuccessors").get_to(graph.mSuccessors);
  j.at("misc").get_to(graph.misc);
}

void to_json(json& j, const RailPoint& point) {
  j = json{{"position", point.position}, {"params", point.params}};
}

void from_json(const json& j, RailPoint& point) {
  j.at("position").get_to(point.position);
  j.at("params").get_to(point.params);
}

void to_json(json& j, const Rail& rail) {
  j = json{{"interpolation", static_cast<int>(rail.interpolation)},
           {"loopPolicy", static_cast<int>(rail.loopPolicy)},
           {"points", rail.points}};
}

void from_json(const json& j, Rail& rail) {
  rail.interpolation =
      static_cast<Interpolation>(j.at("interpolation").get<int>());
  rail.loopPolicy = static_cast<LoopPolicy>(j.at("loopPolicy").get<int>());
  j.at("points").get_to(rail.points);
}
void to_json(json& j, const RespawnPoint& point) {
  j = json{{"position", point.position},
           {"rotation", point.rotation},
           {"id", point.id},
           {"range", point.range}};
}

void from_json(const json& j, RespawnPoint& point) {
  j.at("position").get_to(point.position);
  j.at("rotation").get_to(point.rotation);
  j.at("id").get_to(point.id);
  j.at("range").get_to(point.range);
}

void to_json(json& j, const Stage::lensFlareOptions_t& options) {
  j = json{
      {"a", options.a}, {"r", options.r}, {"g", options.g}, {"b", options.b}};
}

void from_json(const json& j, Stage::lensFlareOptions_t& options) {
  j.at("a").get_to(options.a);
  j.at("r").get_to(options.r);
  j.at("g").get_to(options.g);
  j.at("b").get_to(options.b);
}

void to_json(json& j, const Stage& stage) {
  j = json{{"mLapCount", stage.mLapCount},
           {"mCorner", static_cast<int>(stage.mCorner)},
           {"mStartPosition", static_cast<int>(stage.mStartPosition)},
           {"mFlareTobi", stage.mFlareTobi},
           {"mLensFlareOptions", stage.mLensFlareOptions},
           {"mUnk08", stage.mUnk08},
           {"_", stage._},
           {"mSpeedModifier", stage.mSpeedModifier}};
}

void from_json(const json& j, Stage& stage) {
  j.at("mLapCount").get_to(stage.mLapCount);
  stage.mCorner = static_cast<Corner>(j.at("mCorner").get<int>());
  stage.mStartPosition =
      static_cast<StartPosition>(j.at("mStartPosition").get<int>());
  j.at("mFlareTobi").get_to(stage.mFlareTobi);
  j.at("mLensFlareOptions").get_to(stage.mLensFlareOptions);
  j.at("mUnk08").get_to(stage.mUnk08);
  j.at("_").get_to(stage._);
  j.at("mSpeedModifier").get_to(stage.mSpeedModifier);
}

void to_json(json& j, const StartPoint& point) {
  j = json{{"position", point.position},
           {"rotation", point.rotation},
           {"player_index", point.player_index},
           {"_", point._}};
}

void from_json(const json& j, StartPoint& point) {
  j.at("position").get_to(point.position);
  j.at("rotation").get_to(point.rotation);
  j.at("player_index").get_to(point.player_index);
  j.at("_").get_to(point._);
}
void to_json(json& j, const CourseMap& courseMap) {
  j = json{{"mRevision", courseMap.mRevision},
           {"mOpeningPanIndex", courseMap.mOpeningPanIndex},
           {"mVideoPanIndex", courseMap.mVideoPanIndex},
           {"mStartPoints", courseMap.mStartPoints},
           {"mEnemyPaths", courseMap.mEnemyPaths},
           {"mItemPaths", courseMap.mItemPaths},
           {"mCheckPaths", courseMap.mCheckPaths},
           {"mPaths", courseMap.mPaths},
           {"mGeoObjs", courseMap.mGeoObjs},
           {"mAreas", courseMap.mAreas},
           {"mCameras", courseMap.mCameras},
           {"mRespawnPoints", courseMap.mRespawnPoints},
           {"mCannonPoints", courseMap.mCannonPoints},
           {"mStages", courseMap.mStages},
           {"mMissionPoints", courseMap.mMissionPoints}};
}

void from_json(const json& j, CourseMap& courseMap) {
  j.at("mRevision").get_to(courseMap.mRevision);
  j.at("mOpeningPanIndex").get_to(courseMap.mOpeningPanIndex);
  j.at("mVideoPanIndex").get_to(courseMap.mVideoPanIndex);
  j.at("mStartPoints").get_to(courseMap.mStartPoints);
  j.at("mEnemyPaths").get_to(courseMap.mEnemyPaths);
  j.at("mItemPaths").get_to(courseMap.mItemPaths);
  j.at("mCheckPaths").get_to(courseMap.mCheckPaths);
  j.at("mPaths").get_to(courseMap.mPaths);
  j.at("mGeoObjs").get_to(courseMap.mGeoObjs);
  j.at("mAreas").get_to(courseMap.mAreas);
  j.at("mCameras").get_to(courseMap.mCameras);
  j.at("mRespawnPoints").get_to(courseMap.mRespawnPoints);
  j.at("mCannonPoints").get_to(courseMap.mCannonPoints);
  j.at("mStages").get_to(courseMap.mStages);
  j.at("mMissionPoints").get_to(courseMap.mMissionPoints);
}

std::string DumpJSON(const CourseMap& map) {
  nlohmann::json j = map;
  return j.dump();
}

CourseMap LoadJSON(std::string_view map) {
  nlohmann::json j = nlohmann::json::parse(map);
  CourseMap courseMap = j.get<CourseMap>();
  return courseMap;
}

} // namespace librii::kmp
