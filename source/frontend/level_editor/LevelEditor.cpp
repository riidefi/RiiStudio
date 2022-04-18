#include "LevelEditor.hpp"
#include "CppSupport.hpp"
#include "IO.hpp"
#include "KclUtil.hpp"
#include "ObjUtil.hpp"
#include "Transform.hpp"
#include "ViewCube.hpp"
#include <bit>
#include <core/3d/gl.hpp>
#include <core/common.h>
#include <core/util/gui.hpp>
#include <frontend/root.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <imcxx/Widgets.hpp> // imcxx::Combo
#include <librii/gl/Compiler.hpp>
#include <librii/glhelper/Primitives.hpp>
#include <librii/glhelper/ShaderProgram.hpp>
#include <librii/glhelper/Util.hpp>
#include <librii/glhelper/VBOBuilder.hpp>
#include <librii/math/srt3.hpp>
#include <rsl/Rna.hpp>
#include <vendor/ImGuizmo.h>

namespace riistudio::lvl {

struct VersionReport {
  std::string course_kcl_version;
};

VersionReport MakeVersionReport(const librii::kcol::KCollisionData* kcl) {
  return {
      .course_kcl_version = kcl != nullptr ? kcl->version : "",
  };
}

void DrawRenderOptions(RenderOptions& opt) {
  ImGui::Checkbox("Visual Model", &opt.show_brres);
  ImGui::Checkbox("Collision Model", &opt.show_kcl);
  ImGui::Checkbox("Map Model", &opt.show_map);

  std::string desc = "(" + std::to_string(opt.attr_mask.num_enabled()) +
                     ") Attributes Selected";

  if (ImGui::BeginCombo("Flags", desc.c_str())) {
    for (int i = 0; i < 32; ++i) {
      if (!opt.target_attr_all.is_enabled(i))
        continue;

      const auto& info = GetKCLType(i);
      ImGui::CheckboxFlags(info.name.c_str(), &opt.attr_mask.value,
                           opt.attr_mask.get_flag(i));
    }

    ImGui::EndCombo();
  }

  opt.xlu_mode = imcxx::Combo("Translucency", opt.xlu_mode, "Fast\0Fancy\0");
  ImGui::SliderFloat("Collision Alpha", &opt.kcl_alpha, 0.0f, 1.0f);
}
void LevelEditorWindow::openFile(std::span<const u8> buf, std::string path) {
  // Read .szs
  {
    std::string errc;
    auto root_arc = ReadArchive(buf, errc);
    if (!root_arc.has_value()) {
      mErrDisp = errc;
      return;
    }

    mLevel.root_archive = std::move(*root_arc);
    mLevel.og_path = path;
  }

  setName("Level Editor: " + path);

  // Read course_model.brres
  {
    auto course_model_brres = FindFileWithOverloads(
        mLevel.root_archive, {"course_d_model.brres", "course_model.brres"});
    if (course_model_brres.has_value()) {
      mCourseModel = std::make_unique<RenderableBRRES>(ReadBRRES(
          course_model_brres->file_data, course_model_brres->resolved_path));
    }
  }

  // Read vrcorn_model.brres
  {
    auto vrcorn_model_brres = FindFileWithOverloads(
        mLevel.root_archive, {"vrcorn_d_model.brres", "vrcorn_model.brres"});
    if (vrcorn_model_brres.has_value()) {
      mVrcornModel = std::make_unique<RenderableBRRES>(ReadBRRES(
          vrcorn_model_brres->file_data, vrcorn_model_brres->resolved_path));
    }
  }

  // Read map_model.brres
  {
    auto map_model =
        FindFileWithOverloads(mLevel.root_archive, {"map_model.brres"});
    if (map_model.has_value()) {
      mMapModel = std::make_unique<RenderableBRRES>(
          ReadBRRES(map_model->file_data, map_model->resolved_path));
    }
  }

  // Read course.kcl
  {
    auto course_kcl =
        FindFileWithOverloads(mLevel.root_archive, {"course.kcl"});
    if (course_kcl.has_value()) {
      mCourseKcl = ReadKCL(course_kcl->file_data, course_kcl->resolved_path);
    }
  }

  // Init course.kcl
  if (mCourseKcl) {
    mTriangleRenderer.init(*mCourseKcl);
    disp_opts.init(*mCourseKcl);
  }

  // Read course.kmp
  {
    auto course_kmp =
        FindFileWithOverloads(mLevel.root_archive, {"course.kmp"});
    if (course_kmp.has_value()) {
      mKmp = ReadKMP(course_kmp->file_data, course_kmp->resolved_path);
    }
  }

  // Init course.kmp
  if (mKmp) {
    mKmpHistory.init(*mKmp);

    // Place the camera near the starting point, if it exists
    auto& cam = mRenderSettings.mCameraController.mCamera;
    if (!mKmp->mStartPoints.empty()) {
      auto& start = mKmp->mStartPoints[0];
      cam.mEye = start.position + glm::vec3(0.0f, 10'000.0f, 0.0f);
    }
  }

  // Default camera values
  {
    auto& cam = mRenderSettings.mCameraController.mCamera;

    cam.mClipMin = 200.0f;
    cam.mClipMax = 1000000.0f;
    mRenderSettings.mCameraController.mSpeed = 15'000.0f;
  }
}

void LevelEditorWindow::saveFile(std::string path) {
  // Update archive cache
  if (mKmp != nullptr)
    mLevel.root_archive.files["course.kmp"] = WriteKMP(*mKmp);

  // Flush archive cache
  auto szs_buf = WriteArchive(mLevel.root_archive);

  plate::Platform::writeFile(szs_buf, path);
}

static std::optional<std::pair<std::string, std::vector<u8>>>
GatherNodes(Archive& arc) {
  std::optional<std::pair<std::string, std::vector<u8>>> clicked;
  for (auto& f : arc.folders) {
    if (ImGui::TreeNode((f.first + "/").c_str())) {
      GatherNodes(*f.second.get());
      ImGui::TreePop();
    }
  }
  for (auto& f : arc.files) {
    if (ImGui::Selectable(f.first.c_str())) {
      clicked = f;
    }
  }

  return clicked;
}

static void SetupColumn(const char* name, u32 flags = 0,
                        float init_width_or_weight = -1.0f) {
  ImGui::TableSetupColumn(name, flags | ImGuiTableColumnFlags_WidthFixed,
                          init_width_or_weight > 0 ? init_width_or_weight
                                                   : ImGui::GetFontSize() * 6);
}

static void DrawCoordinateCells(glm::vec3* position,
                                glm::vec3* rotation = nullptr,
                                glm::vec3* scale = nullptr) {
  if (position != nullptr) {
    ImGui::TableNextColumn();
    ImGui::InputFloat("#PosX", &position->x);

    ImGui::TableNextColumn();
    ImGui::InputFloat("#PosY", &position->y);

    ImGui::TableNextColumn();
    ImGui::InputFloat("#PosZ", &position->z);
  }

  if (rotation != nullptr) {
    ImGui::TableNextColumn();
    ImGui::InputFloat("#RotX", &rotation->x);

    ImGui::TableNextColumn();
    ImGui::InputFloat("#RotY", &rotation->y);

    ImGui::TableNextColumn();
    ImGui::InputFloat("#RotZ", &rotation->z);
  }

  if (scale != nullptr) {
    ImGui::TableNextColumn();
    ImGui::InputFloat("#SclX", &scale->x);

    ImGui::TableNextColumn();
    ImGui::InputFloat("#SclY", &scale->y);

    ImGui::TableNextColumn();
    ImGui::InputFloat("#SclZ", &scale->z);
  }
}
static void DrawCoordinateCells(glm::vec2* position,
                                glm::vec2* rotation = nullptr,
                                glm::vec2* scale = nullptr) {
  if (position != nullptr) {
    ImGui::TableNextColumn();
    ImGui::InputFloat("#PosX", &position->x);

    ImGui::TableNextColumn();
    ImGui::InputFloat("#PosY", &position->y);
  }

  if (rotation != nullptr) {
    ImGui::TableNextColumn();
    ImGui::InputFloat("#RotX", &rotation->x);

    ImGui::TableNextColumn();
    ImGui::InputFloat("#RotY", &rotation->y);
  }

  if (scale != nullptr) {
    ImGui::TableNextColumn();
    ImGui::InputFloat("#SclX", &scale->x);

    ImGui::TableNextColumn();
    ImGui::InputFloat("#SclY", &scale->y);
  }
}

static void SetupCoordinateColumns(bool position, bool rotation, bool scaling,
                                   bool show_coords,
                                   bool show_coords_changed = true) {
  // First time this is drawn, show_coords_changed will be false, so we rely
  // on the flag
  u32 coord_flags = show_coords ? 0 : ImGuiTableColumnFlags_DefaultHide;
  {
    auto set_vis = [&] {
      // if (show_coords_changed)
      util::SetNextColumnVisible(show_coords);
    };

    if (position) {
      set_vis();
      SetupColumn("Position.x", coord_flags);
      set_vis();
      SetupColumn("Position.y", coord_flags);
      set_vis();
      SetupColumn("Position.z", coord_flags);
    }

    if (rotation) {
      set_vis();
      SetupColumn("Rotation.x", coord_flags);
      set_vis();
      SetupColumn("Rotation.y", coord_flags);
      set_vis();
      SetupColumn("Rotation.z", coord_flags);
    }

    if (scaling) {
      set_vis();
      SetupColumn("Scale.x", coord_flags);
      set_vis();
      SetupColumn("Scale.y", coord_flags);
      set_vis();
      SetupColumn("Scale.z", coord_flags);
    }
  }
}

static void SetupCoordinateColumns_vec2(bool position, bool rotation,
                                        bool scaling, bool show_coords,
                                        bool show_coords_changed = true) {
  // First time this is drawn, show_coords_changed will be false, so we rely
  // on the flag
  u32 coord_flags = show_coords ? 0 : ImGuiTableColumnFlags_DefaultHide;
  {
    auto set_vis = [&] {
      // if (show_coords_changed)
      util::SetNextColumnVisible(show_coords);
    };

    if (position) {
      set_vis();
      SetupColumn("Position.x", coord_flags);
      set_vis();
      SetupColumn("Position.y", coord_flags);
    }

    if (rotation) {
      set_vis();
      SetupColumn("Rotation.x", coord_flags);
      set_vis();
      SetupColumn("Rotation.y", coord_flags);
    }

    if (scaling) {
      set_vis();
      SetupColumn("Scale.x", coord_flags);
      set_vis();
      SetupColumn("Scale.y", coord_flags);
    }
  }
}

static int NumberOfCoordinateColumns(bool position, bool rotation,
                                     bool scaling) {
  return (static_cast<int>(position) + static_cast<int>(rotation) +
          static_cast<int>(scaling)) *
         3;
}

static int NumberOfCoordinateColumns_vec2(bool position, bool rotation,
                                          bool scaling) {
  return (static_cast<int>(position) + static_cast<int>(rotation) +
          static_cast<int>(scaling)) *
         2;
}

static void SetupJGPTColumns(bool show_coords) {
  SetupCoordinateColumns(true, true, false, show_coords);
  SetupColumn("ID");
  SetupColumn("Radius");
}

static int NumJGPTColumns() {
  return 2 + NumberOfCoordinateColumns(true, true, false);
}

static void DrawJGPTCells(librii::kmp::RespawnPoint& p) {
  DrawCoordinateCells(&p.position, &p.rotation);

  {
    ImGui::TableNextColumn();
    int pli = p.id;
    ImGui::InputInt("#Index", &pli);
    p.id = pli;
  }

  {
    ImGui::TableNextColumn();
    int pli = p.range;
    ImGui::InputInt("#Radius", &pli);
    p.range = pli;
  }
}

void LevelEditorWindow::DrawRespawnTable() {
  bool show_coords_changed = ImGui::Checkbox("Show coordinates?", &show_coords);

  static ImGuiTableFlags flags =
      ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH |
      (0 * ImGuiTableFlags_Resizable) | ImGuiTableFlags_RowBg |
      ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Hideable |
      ImGuiTableFlags_SizingFixedFit;

  int delete_index = -1;
  if (ImGui::BeginTable("##RespawnPoints", 3 + NumJGPTColumns(), flags)) {

    // The first column will use the default _WidthStretch when ScrollX is Off
    // and _WidthFixed when ScrollX is On
    SetupColumn("Name", ImGuiTableColumnFlags_NoHide |
                            ImGuiTableColumnFlags_WidthFixed);
    SetupColumn("ID");
    SetupJGPTColumns(show_coords);
    SetupColumn("");
    ImGui::TableHeadersRow();

    for (int i = 0; i < mKmp->mRespawnPoints.size(); ++i) {
      auto& p = mKmp->mRespawnPoints[i];

      util::IDScope g(i);

      ImGui::TableNextRow();

      auto d = util::RAIICustomSelectable(isSelected(&mKmp->mRespawnPoints, i));

      auto str = "Respawn Point #" + std::to_string(i);
      bool op = ImGui::TreeNodeEx(str.c_str(), ImGuiTreeNodeFlags_Leaf);
      if (util::IsNodeSwitchedTo()) {
        selection.clear();
        select(&mKmp->mRespawnPoints, i);
      }
      ImGui::TableNextColumn();
      ImGui::Text("%i", static_cast<int>(i));

      DrawJGPTCells(p);

      ImGui::TableNextColumn();
      if (ImGui::Button("Delete")) {
        delete_index = i;
      }

      if (op)
        ImGui::TreePop();
    }

    ImGui::EndTable();
  }
  if (ImGui::Button("Add")) {
    AddNewToList(mKmp->mRespawnPoints);
  }
  TryDeleteFromListAndUpdateSelection(mKmp->mRespawnPoints, delete_index);
}

static void SetupKTPTColumns(bool show_coords) {
  SetupCoordinateColumns(true, true, false, show_coords);
  SetupColumn("Player Index");
}

static int NumKTPTColumns() {
  return 1 + NumberOfCoordinateColumns(true, true, false);
}

static void DrawKTPTCells(librii::kmp::StartPoint& p) {
  DrawCoordinateCells(&p.position, &p.rotation);

  {
    ImGui::TableNextColumn();
    int pli = p.player_index;
    ImGui::InputInt("#PlIdx", &pli);
    p.player_index = pli;
  }
}

void LevelEditorWindow::DrawStartPointTable() {
  bool show_coords_changed = ImGui::Checkbox("Show coordinates?", &show_coords);

  static ImGuiTableFlags flags =
      ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH |
      ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg |
      ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Hideable;

  int delete_index = -1;
  if (ImGui::BeginTable("##StartPoints", 3 + NumKTPTColumns(), flags)) {

    // The first column will use the default _WidthStretch when ScrollX is Off
    // and _WidthFixed when ScrollX is On
    SetupColumn("Name", ImGuiTableColumnFlags_NoHide);
    SetupColumn("ID");
    SetupKTPTColumns(show_coords);
    SetupColumn("");
    ImGui::TableHeadersRow();

    for (int i = 0; i < mKmp->mStartPoints.size(); ++i) {
      auto& p = mKmp->mStartPoints[i];

      util::IDScope g(i);

      ImGui::TableNextRow();

      auto d = util::RAIICustomSelectable(isSelected(&mKmp->mStartPoints, i));

      auto str = "Start Point #" + std::to_string(i);
      bool op = ImGui::TreeNodeEx(str.c_str(), ImGuiTreeNodeFlags_Leaf);
      if (util::IsNodeSwitchedTo()) {
        selection.clear();
        select(&mKmp->mStartPoints, i);
      }
      ImGui::TableNextColumn();
      ImGui::Text("%i", static_cast<int>(i));

      DrawKTPTCells(p);

      ImGui::TableNextColumn();
      if (ImGui::Button("Delete")) {
        delete_index = i;
      }

      if (op)
        ImGui::TreePop();
    }

    ImGui::EndTable();
  }
  if (ImGui::Button("Add")) {
    mKmp->mStartPoints.emplace_back();
  }
  if (delete_index >= 0) {
    if (isSelected(&mKmp->mStartPoints, delete_index))
      selection.clear();
    mKmp->mStartPoints.erase(mKmp->mStartPoints.begin() + delete_index);
  }
}

static void SetupAREAColumns(bool show_coords) {
  SetupCoordinateColumns(true, true, true, show_coords);
  SetupColumn("Type");
  SetupColumn("Camera Index");
  SetupColumn("Priority");
  SetupColumn("Param[0]");
  SetupColumn("Param[1]");
  SetupColumn("RailID");
  SetupColumn("EnemyLinkID");
}

static int NumAREAColumns() {
  return 7 + NumberOfCoordinateColumns(true, true, true);
}

static void DrawAREACells(librii::kmp::Area& p) {
  DrawCoordinateCells(&p.mModel.mPosition, &p.mModel.mRotation,
                      &p.mModel.mScaling);

  {
    ImGui::TableNextColumn();
    const char* area_types = "Camera\0"
                             "EffectController\0"
                             "FogController\0"
                             "PullController\0"
                             "EnemyFall\0"
                             "MapArea2D\0"
                             "SoundController\0"
                             "TeresaController\0"
                             "ObjClipClassifier\0"
                             "ObjClipDiscriminator\0"
                             "PlayerBoundary\0"
                             "[MK7]\0";
    int tmp = static_cast<int>(p.mType);
    ImGui::Combo("Type", &tmp, area_types);
    p.mType = static_cast<librii::kmp::AreaType>(tmp);
  }
  {
    ImGui::TableNextColumn();

    int cam_idx = p.mCameraIndex;
    ImGui::InputInt("#CamIdx", &cam_idx);
    p.mCameraIndex = cam_idx;
  }

  {
    ImGui::TableNextColumn();

    int tmp = p.mPriority;
    ImGui::InputInt("#Prio", &tmp);
    p.mPriority = tmp;
  }

  {
    ImGui::TableNextColumn();

    int tmp = p.mParameters[0];
    ImGui::InputInt("#Param0", &tmp);
    p.mParameters[0] = tmp;
  }
  {
    ImGui::TableNextColumn();

    int tmp = p.mParameters[1];
    ImGui::InputInt("#Param1", &tmp);
    p.mParameters[1] = tmp;
  }

  {
    ImGui::TableNextColumn();

    int tmp = p.mRailID;
    ImGui::InputInt("#RailID", &tmp);
    p.mRailID = tmp;
  }

  {
    ImGui::TableNextColumn();

    int tmp = p.mEnemyLinkID;
    ImGui::InputInt("#EnemyLinkID", &tmp);
    p.mEnemyLinkID = tmp;
  }
}

void LevelEditorWindow::DrawAreaTable() {
  bool show_coords_changed = ImGui::Checkbox("Show coordinates?", &show_coords);

  static ImGuiTableFlags flags =
      ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH |
      ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg |
      ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Hideable;

  int delete_index = -1;
  if (ImGui::BeginTable("##AreaTable2", 2 + NumAREAColumns() + 1, flags)) {

    // The first column will use the default _WidthStretch when ScrollX is Off
    // and _WidthFixed when ScrollX is On
    SetupColumn("Name", ImGuiTableColumnFlags_NoHide);
    SetupColumn("ID");

    SetupAREAColumns(show_coords);

    SetupColumn("");
    ImGui::TableHeadersRow();

    for (int i = 0; i < mKmp->mAreas.size(); ++i) {
      auto& p = mKmp->mAreas[i];

      util::IDScope g(i);

      ImGui::TableNextRow();

      auto d = util::RAIICustomSelectable(isSelected(&mKmp->mAreas, i));

      auto str = "Area #" + std::to_string(i);
      bool op = ImGui::TreeNodeEx(str.c_str(), ImGuiTreeNodeFlags_Leaf);
      if (util::IsNodeSwitchedTo()) {
        selection.clear();
        select(&mKmp->mAreas, i);
      }
      ImGui::TableNextColumn();
      ImGui::Text("%i", static_cast<int>(i));

      DrawAREACells(p);

      ImGui::TableNextColumn();
      if (ImGui::Button("Delete")) {
        delete_index = i;
      }

      if (op)
        ImGui::TreePop();
    }

    ImGui::EndTable();
  }
  if (ImGui::Button("Add")) {
    mKmp->mAreas.emplace_back();
  }
  if (delete_index >= 0) {
    if (isSelected(&mKmp->mAreas, delete_index))
      selection.clear();
    mKmp->mAreas.erase(mKmp->mAreas.begin() + delete_index);
  }
}

static void SetupCNPTColumns(bool show_coords) {
  SetupCoordinateColumns(true, true, false, show_coords);
  SetupColumn("Effect");
}

static int NumCNPTColumns() {
  return 1 + NumberOfCoordinateColumns(true, true, false);
}

static void DrawCNPTCells(librii::kmp::Cannon& p) {
  DrawCoordinateCells(&p.mPosition, &p.mRotation);

  {
    ImGui::TableNextColumn();
    const char* area_types = "Linear\0"
                             "CurvedA\0"
                             "CurvedB\0";
    int tmp = static_cast<int>(p.mType);
    ImGui::Combo("Effect", &tmp, area_types);
    p.mType = static_cast<librii::kmp::CannonType>(tmp);
  }
}

void LevelEditorWindow::DrawCannonTable() {
  bool show_coords_changed = ImGui::Checkbox("Show coordinates?", &show_coords);

  static ImGuiTableFlags flags =
      ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH |
      ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg |
      ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Hideable;

  int delete_index = -1;
  if (ImGui::BeginTable("##CannonTable2", 2 + NumCNPTColumns() + 1, flags)) {
    // The first column will use the default _WidthStretch when ScrollX is Off
    // and _WidthFixed when ScrollX is On
    SetupColumn("Name", ImGuiTableColumnFlags_NoHide);
    SetupColumn("ID");

    SetupColumn("");
    ImGui::TableHeadersRow();

    for (int i = 0; i < mKmp->mCannonPoints.size(); ++i) {
      auto& p = mKmp->mCannonPoints[i];

      util::IDScope g(i);

      ImGui::TableNextRow();

      auto d = util::RAIICustomSelectable(isSelected(&mKmp->mCannonPoints, i));

      auto str = "Cannon #" + std::to_string(i);
      bool op = ImGui::TreeNodeEx(str.c_str(), ImGuiTreeNodeFlags_Leaf);
      if (util::IsNodeSwitchedTo()) {
        selection.clear();
        select(&mKmp->mCannonPoints, i);
      }
      ImGui::TableNextColumn();
      ImGui::Text("%i", static_cast<int>(i));

      DrawCNPTCells(p);

      ImGui::TableNextColumn();
      if (ImGui::Button("Delete")) {
        delete_index = i;
      }

      if (op)
        ImGui::TreePop();
    }

    ImGui::EndTable();
  }

  if (ImGui::Button("Add")) {
    AddNewToList(mKmp->mCannonPoints);
  }
  TryDeleteFromListAndUpdateSelection(mKmp->mCannonPoints, delete_index);
}

static void SetupMSPTColumns(bool show_coords) {
  SetupCoordinateColumns(true, true, false, show_coords);
  SetupColumn("LowID");
  SetupColumn("FieldB");
}

static int NumMSPTColumns() {
  return 2 + NumberOfCoordinateColumns(true, true, false);
}

static void DrawMSPTCells(librii::kmp::MissionPoint& p) {
  DrawCoordinateCells(&p.position, &p.rotation);

  {
    ImGui::TableNextColumn();
    int tmp = static_cast<int>(p.id);
    ImGui::InputInt("ID", &tmp);
    p.id = tmp;
  }
  {
    ImGui::TableNextColumn();
    int tmp = static_cast<int>(p.unknown);
    ImGui::InputInt("Unk", &tmp);
    p.unknown = tmp;
  }
}

void LevelEditorWindow::DrawMissionTable() {
  bool show_coords_changed = ImGui::Checkbox("Show coordinates?", &show_coords);

  static ImGuiTableFlags flags =
      ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH |
      ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg |
      ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Hideable;

  int delete_index = -1;
  if (ImGui::BeginTable("##MissionTable2", NumMSPTColumns() + 3, flags)) {

    // The first column will use the default _WidthStretch when ScrollX is Off
    // and _WidthFixed when ScrollX is On
    SetupColumn("Name", ImGuiTableColumnFlags_NoHide);
    SetupColumn("ID");
    SetupMSPTColumns(show_coords);

    SetupColumn("");
    ImGui::TableHeadersRow();

    for (int i = 0; i < mKmp->mMissionPoints.size(); ++i) {
      auto& p = mKmp->mMissionPoints[i];

      util::IDScope g(i);

      ImGui::TableNextRow();

      auto d = util::RAIICustomSelectable(isSelected(&mKmp->mMissionPoints, i));

      auto str = "Cannon #" + std::to_string(i);
      bool op = ImGui::TreeNodeEx(str.c_str(), ImGuiTreeNodeFlags_Leaf);
      if (util::IsNodeSwitchedTo()) {
        selection.clear();
        select(&mKmp->mCannonPoints, i);
      }
      ImGui::TableNextColumn();
      ImGui::Text("%i", static_cast<int>(i));

      DrawMSPTCells(p);

      ImGui::TableNextColumn();
      if (ImGui::Button("Delete")) {
        delete_index = i;
      }

      if (op)
        ImGui::TreePop();
    }

    ImGui::EndTable();
  }

  if (ImGui::Button("Add")) {
    AddNewToList(mKmp->mMissionPoints);
  }
  TryDeleteFromListAndUpdateSelection(mKmp->mMissionPoints, delete_index);
}

static void SetupSTGIColumns() {
  SetupColumn("LapCount");
  SetupColumn("Corner");
  SetupColumn("StartPosition");
  SetupColumn("FlareTobi");
  SetupColumn("FlareColor");
  SetupColumn("mUnk08");
  SetupColumn("SpeedMod");
}

static int NumSTGIColumns() { return 7; }

static void DrawSTGICells(librii::kmp::Stage& p) {
  {
    ImGui::TableNextColumn();
    int tmp = static_cast<int>(p.mLapCount);
    ImGui::InputInt("LapCount", &tmp, 1, 2);
    p.mLapCount = tmp;
  }
  {
    ImGui::TableNextColumn();
    int tmp = static_cast<int>(p.mCorner);
    const char* combo_opts = "Left\0"
                             "Right\0";
    ImGui::Combo("Corner", &tmp, combo_opts);
    p.mCorner = static_cast<librii::kmp::Corner>(tmp);
  }
  {
    ImGui::TableNextColumn();
    int tmp = static_cast<int>(p.mStartPosition);
    const char* combo_opts = "Standard\0"
                             "Near\0";
    ImGui::Combo("StartPosition", &tmp, combo_opts);
    p.mStartPosition = static_cast<librii::kmp::StartPosition>(tmp);
  }
  {
    ImGui::TableNextColumn();
    bool tmp = static_cast<bool>(p.mFlareTobi);

    if (ImGui::Checkbox("FlareTobi", &tmp))
      p.mFlareTobi = tmp;
  }
  {
    ImGui::TableNextColumn();

    auto tmp = librii::kmp::DecodeLensFlareOpts(p.mLensFlareOptions);

    const u32 flags = ImGuiColorEditFlags_NoInputs |
                      ImGuiColorEditFlags_AlphaBar |
                      ImGuiColorEditFlags_DefaultOptions_;
    if (ImGui::ColorEdit4("FlareColor", tmp.data(), flags)) {
      p.mLensFlareOptions = librii::kmp::EncodeLensFlareOpts(tmp);
    }
  }
  {
    ImGui::TableNextColumn();
    int tmp = p.mUnk08;

    if (ImGui::InputInt("mUnk08", &tmp))
      p.mUnk08 = tmp;
  }
  {
    ImGui::TableNextColumn();
    float tmp = librii::kmp::DecodeTruncatedBigFloat(p.mSpeedModifier);

    if (ImGui::InputFloat("SpeedMod", &tmp))
      p.mSpeedModifier = librii::kmp::EncodeTruncatedBigFloat(tmp);
  }
}

void LevelEditorWindow::DrawStages() {
  // bool show_coords_changed = ImGui::Checkbox("Show coordinates?",
  // &show_coords);

  static ImGuiTableFlags flags =
      ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH |
      ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg |
      ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Hideable;

  int delete_index = -1;
  if (ImGui::BeginTable("##StageTable2", 2 + NumSTGIColumns() + 1, flags)) {

    // The first column will use the default _WidthStretch when ScrollX is Off
    // and _WidthFixed when ScrollX is On
    SetupColumn("Name", ImGuiTableColumnFlags_NoHide);
    SetupColumn("ID");
    SetupSTGIColumns();

    SetupColumn("");
    ImGui::TableHeadersRow();

    for (int i = 0; i < mKmp->mStages.size(); ++i) {
      auto& p = mKmp->mStages[i];

      util::IDScope g(i);

      ImGui::TableNextRow();

      auto d = util::RAIICustomSelectable(isSelected(&mKmp->mStages, i));

      auto str = "Stage #" + std::to_string(i);
      bool op = ImGui::TreeNodeEx(str.c_str(), ImGuiTreeNodeFlags_Leaf);
      if (util::IsNodeSwitchedTo()) {
        selection.clear();
        select(&mKmp->mStages, i);
      }
      ImGui::TableNextColumn();
      ImGui::Text("%i", static_cast<int>(i));

      DrawSTGICells(p);

      ImGui::TableNextColumn();
      if (ImGui::Button("Delete")) {
        delete_index = i;
      }

      if (op)
        ImGui::TreePop();
    }

    ImGui::EndTable();
  }

  if (ImGui::Button("Add")) {
    AddNewToList(mKmp->mStages);
  }
  TryDeleteFromListAndUpdateSelection(mKmp->mStages, delete_index);
}

// NOTE: Reused for ENPH/ITPH/CKPH
static void SetupPathColumns() {
  for (int i = 0; i < 6; ++i) {
    std::string str = "Pred #" + std::to_string(i);
    SetupColumn(str.c_str(), 0, ImGui::GetFontSize() * 4);
  }
  for (int i = 0; i < 6; ++i) {
    std::string str = "Succ #" + std::to_string(i);
    SetupColumn(str.c_str(), 0, ImGui::GetFontSize() * 4);
  }
  for (int i = 0; i < 2; ++i) {
    std::string str = "Misc #" + std::to_string(i);
    SetupColumn(str.c_str(), 0, ImGui::GetFontSize() * 4);
  }
}

// NOTE: Reused for ENPH/ITPH/CKPH
static int NumPathColumns() {
  const int num_links = 6; // Higher in MK7
  return num_links * 2 /* pred, succ */ + 2 /* user data */;
}

// NOTE: Reused for ENPH/ITPH/CKPH
template <typename T>
static void DrawPathCells(librii::kmp::DirectedGraph<T>& p) {
  for (int j = 0; j < 6; ++j) {
    ImGui::TableNextColumn();
    util::IDScope g(j);
    util::ConditionalHighlight g2(j < p.mPredecessors.size() &&
                                  p.mPredecessors[j] != 0xFF);

    int tmp = j >= p.mPredecessors.size() ? -1 : p.mPredecessors[j];
    if (ImGui::InputInt("Pred", &tmp)) {
      if (j >= p.mPredecessors.size())
        p.mPredecessors.resize(j + 1);
      p.mPredecessors[j] = tmp;
    }
  }

  for (int j = 0; j < 6; ++j) {
    ImGui::TableNextColumn();
    util::IDScope g(j);
    util::ConditionalHighlight g2(j < p.mSuccessors.size() &&
                                  p.mSuccessors[j] != 0xFF);

    int tmp = j >= p.mSuccessors.size() ? -1 : p.mSuccessors[j];
    if (ImGui::InputInt("Succ", &tmp)) {
      if (j >= p.mSuccessors.size())
        p.mSuccessors.resize(j + 1);
      p.mSuccessors[j] = tmp;
    }
  }

  for (int j = 0; j < 2; ++j) {
    ImGui::TableNextColumn();
    util::IDScope g(j);

    int tmp = p.misc[j];
    if (ImGui::InputInt("UserData", &tmp))
      p.misc[j] = tmp;
  }
}

static void SetupENPHColumns() { SetupPathColumns(); }
static int NumENPHColumns() { return NumPathColumns(); }
static void DrawENPHCells(librii::kmp::EnemyPath& p) { DrawPathCells(p); }

void LevelEditorWindow::DrawEnemyPathTable() {
  // bool show_coords_changed = ImGui::Checkbox("Show coordinates?",
  // &show_coords);

  static ImGuiTableFlags flags =
      ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH |
      ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg |
      ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Hideable;

  int delete_index = -1;
  if (ImGui::BeginTable("##EnemyPathTable", NumENPHColumns() + 3, flags)) {

    // The first column will use the default _WidthStretch when ScrollX is Off
    // and _WidthFixed when ScrollX is On
    SetupColumn("Name", ImGuiTableColumnFlags_NoHide);
    SetupColumn("ID");

    SetupENPHColumns();

    SetupColumn("");
    ImGui::TableHeadersRow();

    for (int i = 0; i < mKmp->mEnemyPaths.size(); ++i) {
      auto& p = mKmp->mEnemyPaths[i];

      util::IDScope g(i);

      ImGui::TableNextRow();

      auto d = util::RAIICustomSelectable(isSelected(&mKmp->mEnemyPaths, i));

      auto str = "EnemyPath #" + std::to_string(i);
      bool op = ImGui::TreeNodeEx(str.c_str(), ImGuiTreeNodeFlags_Leaf);
      if (util::IsNodeSwitchedTo()) {
        selection.clear();
        select(&mKmp->mEnemyPaths, i);
      }
      ImGui::TableNextColumn();
      ImGui::Text("%i", static_cast<int>(i));

      DrawENPHCells(p);

      ImGui::TableNextColumn();
      if (ImGui::Button("Delete")) {
        delete_index = i;
      }

      if (op)
        ImGui::TreePop();
    }

    ImGui::EndTable();
  }

  if (ImGui::Button("Add")) {
    AddNewToList(mKmp->mEnemyPaths);
  }
  TryDeleteFromListAndUpdateSelection(mKmp->mEnemyPaths, delete_index);
}

static void SetupITPHColumns() { SetupPathColumns(); }
static int NumITPHColumns() { return NumPathColumns(); }
static void DrawITPHCells(librii::kmp::ItemPath& p) { DrawPathCells(p); }

void LevelEditorWindow::DrawItemPathTable() {
  // bool show_coords_changed = ImGui::Checkbox("Show coordinates?",
  // &show_coords);

  static ImGuiTableFlags flags =
      ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH |
      ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg |
      ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Hideable;

  int delete_index = -1;
  if (ImGui::BeginTable("##ItemPathTable", NumITPHColumns() + 3, flags)) {

    // The first column will use the default _WidthStretch when ScrollX is Off
    // and _WidthFixed when ScrollX is On
    SetupColumn("Name", ImGuiTableColumnFlags_NoHide);
    SetupColumn("ID");

    SetupITPHColumns();

    SetupColumn("");
    ImGui::TableHeadersRow();

    for (int i = 0; i < mKmp->mItemPaths.size(); ++i) {
      auto& p = mKmp->mItemPaths[i];

      util::IDScope g(i);

      ImGui::TableNextRow();

      auto d = util::RAIICustomSelectable(isSelected(&mKmp->mItemPaths, i));

      auto str = "ItemPath #" + std::to_string(i);
      bool op = ImGui::TreeNodeEx(str.c_str(), ImGuiTreeNodeFlags_Leaf);
      if (util::IsNodeSwitchedTo()) {
        selection.clear();
        select(&mKmp->mItemPaths, i);
      }
      ImGui::TableNextColumn();
      ImGui::Text("%i", static_cast<int>(i));

      DrawITPHCells(p);

      ImGui::TableNextColumn();
      if (ImGui::Button("Delete")) {
        delete_index = i;
      }

      if (op)
        ImGui::TreePop();
    }

    ImGui::EndTable();
  }

  if (ImGui::Button("Add")) {
    AddNewToList(mKmp->mItemPaths);
  }
  TryDeleteFromListAndUpdateSelection(mKmp->mItemPaths, delete_index);
}

static void SetupCKPHColumns() { SetupPathColumns(); }
static int NumCKPHColumns() { return NumPathColumns(); }
static void DrawCKPHCells(librii::kmp::CheckPath& p) { DrawPathCells(p); }

void LevelEditorWindow::DrawCheckPathTable() {
  // bool show_coords_changed = ImGui::Checkbox("Show coordinates?",
  // &show_coords);

  static ImGuiTableFlags flags =
      ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH |
      ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg |
      ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Hideable;

  int delete_index = -1;
  if (ImGui::BeginTable("##CheckPathTable", NumCKPHColumns() + 3, flags)) {

    // The first column will use the default _WidthStretch when ScrollX is Off
    // and _WidthFixed when ScrollX is On
    SetupColumn("Name", ImGuiTableColumnFlags_NoHide);
    SetupColumn("ID");

    SetupCKPHColumns();

    SetupColumn("");
    ImGui::TableHeadersRow();

    for (int i = 0; i < mKmp->mCheckPaths.size(); ++i) {
      auto& p = mKmp->mCheckPaths[i];

      util::IDScope g(i);

      ImGui::TableNextRow();

      auto d = util::RAIICustomSelectable(isSelected(&mKmp->mCheckPaths, i));

      auto str = "CheckPaths #" + std::to_string(i);
      bool op = ImGui::TreeNodeEx(str.c_str(), ImGuiTreeNodeFlags_Leaf);
      if (util::IsNodeSwitchedTo()) {
        selection.clear();
        select(&mKmp->mCheckPaths, i);
      }
      ImGui::TableNextColumn();
      ImGui::Text("%i", static_cast<int>(i));

      DrawCKPHCells(p);

      ImGui::TableNextColumn();
      if (ImGui::Button("Delete")) {
        delete_index = i;
      }

      if (op)
        ImGui::TreePop();
    }

    ImGui::EndTable();
  }

  if (ImGui::Button("Add")) {
    AddNewToList(mKmp->mCheckPaths);
  }
  TryDeleteFromListAndUpdateSelection(mKmp->mCheckPaths, delete_index);
}

static void SetupENPTColumns(bool show_coords) {
  SetupCoordinateColumns(true, false, false, show_coords);
  SetupColumn("Radius");
  SetupColumn("Param[0]");
  SetupColumn("Param[1]");
  SetupColumn("Param[2]");
  SetupColumn("Param[3]");
}

static int NumENPTColumns() {
  return NumberOfCoordinateColumns(true, false, false) + 5;
}

static void DrawENPTCells(librii::kmp::EnemyPoint& p) {
  DrawCoordinateCells(&p.position);

  {
    ImGui::TableNextColumn();
    ImGui::InputFloat("Radius", &p.deviation);
  }

  for (int j = 0; j < 4; ++j) {
    ImGui::TableNextColumn();
    util::IDScope g(j);

    int tmp = static_cast<int>(p.param[j]);
    if (ImGui::InputInt("Param", &tmp))
      p.param[j] = tmp;
  }
}

void LevelEditorWindow::DrawEnemyPointTable(librii::kmp::EnemyPath& path) {
  bool show_coords_changed = ImGui::Checkbox("Show coordinates?", &show_coords);

  static ImGuiTableFlags flags =
      ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH |
      ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg |
      ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Hideable;

  int delete_index = -1;
  if (ImGui::BeginTable("##EnemyPointTable", 2 + NumENPTColumns() + 1, flags)) {

    // The first column will use the default _WidthStretch when ScrollX is Off
    // and _WidthFixed when ScrollX is On
    SetupColumn("Name", ImGuiTableColumnFlags_NoHide);
    SetupColumn("ID");

    SetupENPTColumns(show_coords);

    SetupColumn("");
    ImGui::TableHeadersRow();

    for (int i = 0; i < path.mPoints.size(); ++i) {
      auto& p = path.mPoints[i];

      util::IDScope g(i);

      ImGui::TableNextRow();

      auto d = util::RAIICustomSelectable(isSelected(&path.mPoints, i));

      auto str = "Point #" + std::to_string(i);
      bool op = ImGui::TreeNodeEx(str.c_str(), ImGuiTreeNodeFlags_Leaf);
      if (util::IsNodeSwitchedTo()) {
        selection.clear();
        select(&path.mPoints, i);
      }
      ImGui::TableNextColumn();
      ImGui::Text("%i", static_cast<int>(i));

      DrawENPTCells(p);

      ImGui::TableNextColumn();
      if (ImGui::Button("Delete")) {
        delete_index = i;
      }

      if (op)
        ImGui::TreePop();
    }

    ImGui::EndTable();
  }

  if (ImGui::Button("Add")) {
    AddNewToList(path.mPoints);
  }
  TryDeleteFromListAndUpdateSelection(path.mPoints, delete_index);
}

static void SetupITPTColumns(bool show_coords) {
  SetupCoordinateColumns(true, false, false, show_coords);
  SetupColumn("Radius");
  SetupColumn("Param[0]");
  SetupColumn("Param[1]");
  SetupColumn("Param[2]");
  SetupColumn("Param[3]");
}

static int NumITPTColumns() {
  return NumberOfCoordinateColumns(true, false, false) + 5;
}

static void DrawITPTCells(librii::kmp::ItemPoint& p) {
  DrawCoordinateCells(&p.position);

  {
    ImGui::TableNextColumn();
    ImGui::InputFloat("Radius", &p.deviation);
  }

  for (int j = 0; j < 4; ++j) {
    ImGui::TableNextColumn();
    util::IDScope g(j);

    int tmp = static_cast<int>(p.param[j]);
    if (ImGui::InputInt("Param", &tmp))
      p.param[j] = tmp;
  }
}

void LevelEditorWindow::DrawItemPointTable(librii::kmp::ItemPath& path) {
  bool show_coords_changed = ImGui::Checkbox("Show coordinates?", &show_coords);

  static ImGuiTableFlags flags =
      ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH |
      ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg |
      ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Hideable;

  int delete_index = -1;
  if (ImGui::BeginTable("##ItemPointTable", 2 + NumITPTColumns() + 1, flags)) {
    SetupColumn("Name", ImGuiTableColumnFlags_NoHide);
    SetupColumn("ID");
    SetupITPTColumns(show_coords);
    SetupColumn("");
    ImGui::TableHeadersRow();

    for (int i = 0; i < path.mPoints.size(); ++i) {
      auto& p = path.mPoints[i];

      util::IDScope g(i);

      ImGui::TableNextRow();

      auto d = util::RAIICustomSelectable(isSelected(&path.mPoints, i));

      auto str = "Point #" + std::to_string(i);
      bool op = ImGui::TreeNodeEx(str.c_str(), ImGuiTreeNodeFlags_Leaf);
      if (util::IsNodeSwitchedTo()) {
        selection.clear();
        select(&path.mPoints, i);
      }
      ImGui::TableNextColumn();
      ImGui::Text("%i", static_cast<int>(i));

      DrawITPTCells(p);

      ImGui::TableNextColumn();
      if (ImGui::Button("Delete")) {
        delete_index = i;
      }

      if (op)
        ImGui::TreePop();
    }

    ImGui::EndTable();
  }

  if (ImGui::Button("Add")) {
    AddNewToList(path.mPoints);
  }
  TryDeleteFromListAndUpdateSelection(path.mPoints, delete_index);
}

static void SetupCKPTColumns(bool show_coords) {
  SetupCoordinateColumns_vec2(true, false, false, show_coords);
  SetupCoordinateColumns_vec2(true, false, false, show_coords);
  SetupColumn("RespawnIndex");
  SetupColumn("LapCheck");
}

static int NumCKPTColumns() {
  return NumberOfCoordinateColumns_vec2(true, false, false) * 2 + 2;
}

static void DrawCKPTCells(librii::kmp::CheckPoint& p) {
  DrawCoordinateCells(&p.mLeft);
  DrawCoordinateCells(&p.mRight);

  {
    ImGui::TableNextColumn();

    int tmp = p.mRespawnIndex;
    if (ImGui::InputInt("RespawnIndex", &tmp))
      p.mRespawnIndex = tmp;
  }

  {
    ImGui::TableNextColumn();

    int tmp = p.mLapCheck;
    if (ImGui::InputInt("LapCheck", &tmp))
      p.mLapCheck = tmp;
  }
}

static void PushLapCheckStyle() {
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{1.0f, 0.0f, 0.0f, 1.0f});
}
static void PopLapCheckStyle() { ImGui::PopStyleColor(); }

void LevelEditorWindow::DrawCheckPointTable(librii::kmp::CheckPath& path) {
  bool show_coords_changed = ImGui::Checkbox("Show coordinates?", &show_coords);

  static ImGuiTableFlags flags =
      ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH |
      ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg |
      ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Hideable;

  int delete_index = -1;
  if (ImGui::BeginTable("##CheckPointTable", 2 + NumCKPTColumns() + 1, flags)) {
    SetupColumn("Name", ImGuiTableColumnFlags_NoHide);
    SetupColumn("ID");
    SetupCKPTColumns(show_coords);
    SetupColumn("");
    ImGui::TableHeadersRow();

    for (int i = 0; i < path.mPoints.size(); ++i) {
      auto& p = path.mPoints[i];

      util::IDScope g(i);

      ImGui::TableNextRow();

      const bool is_lapcheck = librii::kmp::IsLapCheck(p);

      if (is_lapcheck)
        PushLapCheckStyle();

      bool op;
      {
        auto d = util::RAIICustomSelectable(isSelected(&path.mPoints, i));

        auto str = "Point #" + std::to_string(i);
        op = ImGui::TreeNodeEx(str.c_str(), ImGuiTreeNodeFlags_Leaf);
        if (util::IsNodeSwitchedTo()) {
          selection.clear();
          select(&path.mPoints, i);
        }
        ImGui::TableNextColumn();
        ImGui::Text("%i", static_cast<int>(i));

        DrawCKPTCells(p);

        ImGui::TableNextColumn();
        if (ImGui::Button("Delete")) {
          delete_index = i;
        }
      }
      if (is_lapcheck)
        PopLapCheckStyle();

      if (op)
        ImGui::TreePop();
    }

    ImGui::EndTable();
  }

  if (ImGui::Button("Add")) {
    AddNewToList(path.mPoints);
  }
  TryDeleteFromListAndUpdateSelection(path.mPoints, delete_index);
}

void LevelEditorWindow::draw_() {
  if (mErrDisp.size()) {
    util::PushErrorSyle();
    ImGui::Text("ERROR: %s", mErrDisp.c_str());
    util::PopErrorStyle();
    return;
  }

  if (ImGui::BeginMenuBar()) {
    if (ImGui::Button("Save")) {
      saveFile("umm.szs");
    }

    ImGui::EndMenuBar();
  }
  if (Begin(".szs", nullptr, 0, this)) {
    auto clicked = GatherNodes(mLevel.root_archive);
    if (clicked.has_value()) {
      auto* pParent = frontend::RootWindow::spInstance;
      if (pParent) {
        pParent->dropDirect(clicked->second, clicked->first);
      }
    }
  }
  ImGui::End();

  if (Begin("View", nullptr, ImGuiWindowFlags_MenuBar, this)) {
    auto bounds = ImGui::GetWindowSize();
    if (mViewport.begin(static_cast<u32>(bounds.x),
                        static_cast<u32>(bounds.y))) {
      drawScene(bounds.x, bounds.y);

      mViewport.end();
    }
  }
  ImGui::End();

  if (Begin("Debug Info", &mDrawDebug, 0, this)) {
    DrawCppSupport();
    // auto versions = GetCppSupport();
    // ImGui::TextUnformatted(versions.data(), versions.data() +
    // versions.size());
  }
  End();

  if (mKmp == nullptr)
    return;

  if (Begin("Tree", nullptr, ImGuiWindowFlags_MenuBar, this)) {
    static ImGuiTableFlags flags =
        ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH |
        ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg;
    // ImGui::CheckboxFlags("ImGuiTableFlags_Scroll", (unsigned int*)&flags,
    // ImGuiTableFlags_Scroll);
    // ImGui::CheckboxFlags("ImGuiTableFlags_ScrollFreezeLeftColumn", (unsigned
    // int*)&flags, ImGuiTableFlags_ScrollFreezeLeftColumn);

    if (ImGui::BeginTable("##3ways", 2, flags)) {
      // The first column will use the default _WidthStretch when ScrollX is Off
      // and _WidthFixed when ScrollX is On
      SetupColumn("Name", ImGuiTableColumnFlags_NoHide);
      SetupColumn("ID");
      // SetupColumn("Type", ImGuiTableColumnFlags_WidthFixed,
      //                        ImGui::GetFontSize() * 10);
      ImGui::TableHeadersRow();

      {
        ImGui::TableNextRow();

        auto d = util::RAIICustomSelectable(mPage == Page::StartPoints);

        if (ImGui::TreeNodeEx("Start Points", ImGuiTreeNodeFlags_Leaf)) {
          ImGui::TreePop();
        }
        if (util::IsNodeSwitchedTo()) {
          mPage = Page::StartPoints;
        }
        ImGui::TableNextColumn();
        ImGui::Text("%i", static_cast<int>(mKmp->mStartPoints.size()));
      }

      {
        bool has_child = false;
        {
          ImGui::TableNextRow();
          auto d = util::RAIICustomSelectable(mPage == Page::EnemyPaths);

          has_child = ImGui::TreeNodeEx("Enemy Paths", 0);
          if (util::IsNodeSwitchedTo()) {
            mPage = Page::EnemyPaths;
          }

          ImGui::TableNextColumn();
          ImGui::Text("%i", static_cast<int>(mKmp->mEnemyPaths.size()));
        }

        if (has_child) {
          for (int i = 0; i < mKmp->mEnemyPaths.size(); ++i) {
            ImGui::TableNextRow();

            auto d = util::RAIICustomSelectable(mPage == Page::EnemyPaths_Sub &&
                                                mSubPageID == i);

            std::string node_s = "Enemy Path #" + std::to_string(i);
            if (ImGui::TreeNodeEx(node_s.c_str(), ImGuiTreeNodeFlags_Leaf)) {
              ImGui::TreePop();
            }

            if (util::IsNodeSwitchedTo()) {
              mPage = Page::EnemyPaths_Sub;
              mSubPageID = i;
            }
          }

          ImGui::TreePop();
        }
      }

      {
        bool has_child = false;
        {
          ImGui::TableNextRow();
          auto d = util::RAIICustomSelectable(mPage == Page::ItemPaths);

          has_child = ImGui::TreeNodeEx("Item Paths", 0);
          if (util::IsNodeSwitchedTo()) {
            mPage = Page::ItemPaths;
          }

          ImGui::TableNextColumn();
          ImGui::Text("%i", static_cast<int>(mKmp->mItemPaths.size()));
        }

        if (has_child) {
          for (int i = 0; i < mKmp->mItemPaths.size(); ++i) {
            ImGui::TableNextRow();

            auto d = util::RAIICustomSelectable(mPage == Page::ItemPaths_Sub &&
                                                mSubPageID == i);

            std::string node_s = "Item Path #" + std::to_string(i);
            if (ImGui::TreeNodeEx(node_s.c_str(), ImGuiTreeNodeFlags_Leaf)) {
              ImGui::TreePop();
            }

            if (util::IsNodeSwitchedTo()) {
              mPage = Page::ItemPaths_Sub;
              mSubPageID = i;
            }
          }

          ImGui::TreePop();
        }
      }

      {
        bool has_child = false;
        {
          ImGui::TableNextRow();
          auto d = util::RAIICustomSelectable(mPage == Page::CheckPaths);

          has_child = ImGui::TreeNodeEx("Check Paths", 0);
          if (util::IsNodeSwitchedTo()) {
            mPage = Page::CheckPaths;
          }

          ImGui::TableNextColumn();
          ImGui::Text("%i", static_cast<int>(mKmp->mCheckPaths.size()));
        }

        if (has_child) {
          for (int i = 0; i < mKmp->mCheckPaths.size(); ++i) {
            ImGui::TableNextRow();

            const bool is_lapcheck = IsLapCheck(mKmp->mCheckPaths[i]);

            if (is_lapcheck)
              PushLapCheckStyle();
            {
              auto d = util::RAIICustomSelectable(
                  mPage == Page::CheckPaths_Sub && mSubPageID == i);

              std::string node_s = "Check Path #" + std::to_string(i);
              if (ImGui::TreeNodeEx(node_s.c_str(), ImGuiTreeNodeFlags_Leaf)) {
                ImGui::TreePop();
              }

              if (util::IsNodeSwitchedTo()) {
                mPage = Page::CheckPaths_Sub;
                mSubPageID = i;
              }
            }
            if (is_lapcheck)
              PopLapCheckStyle();
          }

          ImGui::TreePop();
        }
      }

      {
        ImGui::TableNextRow();

        bool b = util::BeginCustomSelectable(mPage == Page::Objects);
        if (ImGui::TreeNodeEx("Objects", ImGuiTreeNodeFlags_Leaf)) {
          if (util::IsNodeSwitchedTo()) {
            mPage = Page::Objects;
          }
          ImGui::TreePop();
        }
        ImGui::TableNextColumn();
        ImGui::Text("%i", static_cast<int>(mKmp->mGeoObjs.size()));
        util::EndCustomSelectable(b);
      }

      {
        ImGui::TableNextRow();

        bool b = util::BeginCustomSelectable(mPage == Page::Paths);
        if (ImGui::TreeNodeEx("Paths", ImGuiTreeNodeFlags_Leaf)) {
          if (util::IsNodeSwitchedTo()) {
            mPage = Page::Paths;
          }
          ImGui::TreePop();
        }
        ImGui::TableNextColumn();
        ImGui::Text("%i", static_cast<int>(mKmp->mPaths.size()));
        util::EndCustomSelectable(b);
      }

      {
        ImGui::TableNextRow();

        bool b = util::BeginCustomSelectable(mPage == Page::Areas);
        if (ImGui::TreeNodeEx("Areas", ImGuiTreeNodeFlags_Leaf)) {
          if (util::IsNodeSwitchedTo()) {
            mPage = Page::Areas;
          }
          ImGui::TreePop();
        }
        ImGui::TableNextColumn();
        ImGui::Text("%i", static_cast<int>(mKmp->mAreas.size()));
        util::EndCustomSelectable(b);
      }

      {
        ImGui::TableNextRow();

        bool b = util::BeginCustomSelectable(mPage == Page::Cameras);
        if (ImGui::TreeNodeEx("Cameras", ImGuiTreeNodeFlags_Leaf)) {
          if (util::IsNodeSwitchedTo()) {
            mPage = Page::Cameras;
          }
          ImGui::TreePop();
        }
        ImGui::TableNextColumn();
        ImGui::Text("%i", static_cast<int>(mKmp->mCameras.size()));
        util::EndCustomSelectable(b);
      }

      {
        ImGui::TableNextRow();

        bool b = util::BeginCustomSelectable(mPage == Page::Respawns);
        if (ImGui::TreeNodeEx("Respawns", ImGuiTreeNodeFlags_Leaf)) {
          if (util::IsNodeSwitchedTo()) {
            mPage = Page::Respawns;
          }
          ImGui::TreePop();
        }
        ImGui::TableNextColumn();
        ImGui::Text("%i", static_cast<int>(mKmp->mRespawnPoints.size()));
        util::EndCustomSelectable(b);
      }

      {
        ImGui::TableNextRow();

        bool b = util::BeginCustomSelectable(mPage == Page::Cannons);
        if (ImGui::TreeNodeEx("Cannons", ImGuiTreeNodeFlags_Leaf)) {
          if (util::IsNodeSwitchedTo()) {
            mPage = Page::Cannons;
          }
          ImGui::TreePop();
        }
        ImGui::TableNextColumn();
        ImGui::Text("%i", static_cast<int>(mKmp->mCannonPoints.size()));
        util::EndCustomSelectable(b);
      }

      {
        ImGui::TableNextRow();

        bool b = util::BeginCustomSelectable(mPage == Page::MissionPoints);
        if (ImGui::TreeNodeEx("Mission Points", ImGuiTreeNodeFlags_Leaf)) {
          if (util::IsNodeSwitchedTo()) {
            mPage = Page::MissionPoints;
          }
          ImGui::TreePop();
        }
        ImGui::TableNextColumn();
        ImGui::Text("%i", static_cast<int>(mKmp->mMissionPoints.size()));
        util::EndCustomSelectable(b);
      }

      {
        ImGui::TableNextRow();

        bool b = util::BeginCustomSelectable(mPage == Page::Stages);
        if (ImGui::TreeNodeEx("Stages", ImGuiTreeNodeFlags_Leaf)) {
          if (util::IsNodeSwitchedTo()) {
            mPage = Page::Stages;
          }
          ImGui::TreePop();
        }
        ImGui::TableNextColumn();
        ImGui::Text("%i", static_cast<int>(mKmp->mStages.size()));
        util::EndCustomSelectable(b);
      }

      ImGui::TableNextRow();

      ImGui::EndTable();
    }
  }
  ImGui::End();

  if (Begin("Properties", nullptr, ImGuiWindowFlags_MenuBar, this)) {
    switch (mPage) {
    case Page::StartPoints:
      DrawStartPointTable();
      break;
    case Page::EnemyPaths:
      DrawEnemyPathTable();
      break;
    case Page::EnemyPaths_Sub:
      if (mKmp && mSubPageID >= 0 && mSubPageID < mKmp->mEnemyPaths.size()) {
        DrawEnemyPointTable(mKmp->mEnemyPaths[mSubPageID]);
      }
      break;
    case Page::ItemPaths:
      DrawItemPathTable();
      break;
    case Page::ItemPaths_Sub:
      if (mKmp && mSubPageID >= 0 && mSubPageID < mKmp->mItemPaths.size()) {
        DrawItemPointTable(mKmp->mItemPaths[mSubPageID]);
      }
      break;
    case Page::CheckPaths:
      DrawCheckPathTable();
      break;
    case Page::CheckPaths_Sub:
      if (mKmp && mSubPageID >= 0 && mSubPageID < mKmp->mCheckPaths.size()) {
        DrawCheckPointTable(mKmp->mCheckPaths[mSubPageID]);
      }
      break;
    case Page::Areas:
      DrawAreaTable();
      break;
    case Page::Respawns:
      DrawRespawnTable();
      break;
    case Page::Cannons:
      DrawCannonTable();
      break;
    case Page::MissionPoints:
      DrawMissionTable();
      break;
    case Page::Stages:
      DrawStages();
      break;
    }
  }
  ImGui::End();
}

ImGuiID LevelEditorWindow::buildDock(ImGuiID root_id) {
  ImGuiID next = root_id;
  ImGuiID dock_right_id =
      ImGui::DockBuilderSplitNode(next, ImGuiDir_Right, 0.4f, nullptr, &next);
  ImGuiID dock_left_id =
      ImGui::DockBuilderSplitNode(next, ImGuiDir_Left, 0.2f, nullptr, &next);
  ImGuiID dock_right_down_id = ImGui::DockBuilderSplitNode(
      dock_right_id, ImGuiDir_Down, 0.7f, nullptr, &dock_right_id);

  ImGui::DockBuilderDockWindow(idIfyChild("Tree").c_str(), dock_right_id);
  ImGui::DockBuilderDockWindow(idIfyChild("Properties").c_str(),
                               dock_right_down_id);
  ImGui::DockBuilderDockWindow(idIfyChild(".szs").c_str(), dock_left_id);
  ImGui::DockBuilderDockWindow(idIfyChild("View").c_str(), next);
  return next;
}

void PushCube(riistudio::lib3d::SceneState& state, glm::mat4 modelMtx,
              glm::mat4 viewMtx, glm::mat4 projMtx) {

  auto& cube = state.getBuffers().opaque.nodes.emplace_back();
  cube.mega_state = {.cullMode = (u32)-1 /* GL_BACK */,
                     .depthWrite = GL_TRUE,
                     .depthCompare = GL_LEQUAL,
                     .frontFace = GL_CW,

                     .blendMode = GL_FUNC_ADD,
                     .blendSrcFactor = GL_ONE,
                     .blendDstFactor = GL_ZERO};
  cube.shader_id = librii::glhelper::GetCubeShader();
  cube.vao_id = librii::glhelper::GetCubeVAO();
  cube.texture_objects.resize(0);

  cube.primitive_type = librii::gfx::PrimitiveType::Triangles;
  cube.vertex_count = librii::glhelper::GetCubeNumVerts();
  cube.vertex_data_type = librii::gfx::DataType::U32;
  cube.indices = 0; // Offset in VAO

  cube.bound = {};

  glm::mat4 mvp = projMtx * viewMtx * modelMtx;
  librii::gl::UniformSceneParams params{.projection = mvp,
                                        .Misc0 = {69.0f, 0.0f, 0.0f, 0.0f}};

  enum {
    // So UBOBuilder requires the same UBO layout for every use of the same
    // binding point. It also requires there to be 4x binding point 0, 4x
    // binding point 1, etc.. except it doesn't break if we have extra on
    // binding point 0. This is a mess
    UB_SCENEPARAMS_FOR_CUBE_ID = 0
  };

  auto& uniforms =
      cube.uniform_data.emplace_back(librii::gfx::SceneNode::UniformData{
          .binding_point = UB_SCENEPARAMS_FOR_CUBE_ID,
          .raw_data = {sizeof(params)}});

  uniforms.raw_data.resize(sizeof(params));
  memcpy(uniforms.raw_data.data(), &params, sizeof(params));

  glUniformBlockBinding(
      cube.shader_id, glGetUniformBlockIndex(cube.shader_id, "ub_SceneParams"),
      UB_SCENEPARAMS_FOR_CUBE_ID);

  int query_min = 1024;
  glGetActiveUniformBlockiv(cube.shader_id, UB_SCENEPARAMS_FOR_CUBE_ID,
                            GL_UNIFORM_BLOCK_DATA_SIZE, &query_min);
  // This conflicts with the previous min?

  cube.uniform_mins.push_back(librii::gfx::SceneNode::UniformMin{
      .binding_point = UB_SCENEPARAMS_FOR_CUBE_ID,
      .min_size = static_cast<u32>(query_min)});
}

struct Manipulator {
  bool dgui = true;
  ImGuizmo::OPERATION mCurrentGizmoOperation = ImGuizmo::ROTATE;
  ImGuizmo::MODE mCurrentGizmoMode = ImGuizmo::WORLD;
  bool useSnap = false;
  float st[3];
  float sr[3];
  float ss[3];
  float* snap = st;

  void drawUi(glm::mat4& mx) {
    if (ImGui::IsKeyPressed(65 + 'G' - 'A'))
      mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    if (ImGui::IsKeyPressed(65 + 'R' - 'A'))
      mCurrentGizmoOperation = ImGuizmo::ROTATE;
    if (ImGui::IsKeyPressed(65 + 'T' - 'A'))
      mCurrentGizmoOperation = ImGuizmo::SCALE;
    if (ImGui::RadioButton("Translate",
                           mCurrentGizmoOperation == ImGuizmo::TRANSLATE))
      mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    ImGui::SameLine();
    if (ImGui::RadioButton("Rotate",
                           mCurrentGizmoOperation == ImGuizmo::ROTATE))
      mCurrentGizmoOperation = ImGuizmo::ROTATE;
    ImGui::SameLine();
    if (ImGui::RadioButton("Scale", mCurrentGizmoOperation == ImGuizmo::SCALE))
      mCurrentGizmoOperation = ImGuizmo::SCALE;
    float matrixTranslation[3], matrixRotation[3], matrixScale[3];
    ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(mx), matrixTranslation,
                                          matrixRotation, matrixScale);
    ImGui::InputFloat3("Tr", matrixTranslation);
    ImGui::InputFloat3("Rt", matrixRotation);
    ImGui::InputFloat3("Sc", matrixScale);
    ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation,
                                            matrixScale, glm::value_ptr(mx));

    if (mCurrentGizmoOperation != ImGuizmo::SCALE) {
      if (ImGui::RadioButton("Local", mCurrentGizmoMode == ImGuizmo::LOCAL))
        mCurrentGizmoMode = ImGuizmo::LOCAL;
      ImGui::SameLine();
      if (ImGui::RadioButton("World", mCurrentGizmoMode == ImGuizmo::WORLD))
        mCurrentGizmoMode = ImGuizmo::WORLD;
    }
    if (ImGui::IsKeyPressed(83))
      useSnap = !useSnap;
    ImGui::Checkbox("SNAP", &useSnap);
    ImGui::SameLine();
    switch (mCurrentGizmoOperation) {
    case ImGuizmo::TRANSLATE:
      snap = st;
      ImGui::InputFloat3("Snap", st);
      break;
    case ImGuizmo::ROTATE:
      snap = sr;
      ImGui::InputFloat("Angle Snap", sr);
      break;
    case ImGuizmo::SCALE:
      snap = ss;
      ImGui::InputFloat("Scale Snap", ss);
      break;
    default:
      snap = st;
    }
  }

  bool manipulate(glm::mat4& mx, const glm::mat4& viewMtx,
                  const glm::mat4& projMtx) {
    return ImGuizmo::Manipulate(glm::value_ptr(viewMtx),
                                glm::value_ptr(projMtx), mCurrentGizmoOperation,
                                mCurrentGizmoMode, glm::value_ptr(mx), NULL,
                                useSnap ? snap : NULL);
  }

  void drawDebugCube(const glm::mat4& mx, const glm::mat4& viewMtx,
                     const glm::mat4& projMtx) {
    ImGuizmo::DrawCubes(glm::value_ptr(viewMtx), glm::value_ptr(projMtx),
                        glm::value_ptr(mx), 1);
  }
};

void LevelEditorWindow::drawScene(u32 width, u32 height) {
  mRenderSettings.drawMenuBar(false, false);

  // if (ImGui::BeginMenuBar()) {
  DrawRenderOptions(disp_opts);
  auto report = MakeVersionReport(mCourseKcl.get());
  ImGui::Text("KCL: %s", report.course_kcl_version.c_str());
  //   ImGui::EndMenuBar();
  // }

  if (!mRenderSettings.rend)
    return;

  const auto time_step = mDeltaTimer.tick();
  if (mMouseHider.begin_interaction(ImGui::IsWindowFocused())) {
    const auto input_state = frontend::buildInputState();
    mRenderSettings.mCameraController.move(time_step, input_state);

    mMouseHider.end_interaction(input_state.clickView);
  }

  // Only configure once we have stuff
  if (mSceneState.getBuffers().opaque.nodes.size()) {
    frontend::ConfigureCameraControllerByBounds(
        mRenderSettings.mCameraController, mSceneState.computeBounds(), 1.0f);
    auto& cam = mRenderSettings.mCameraController.mCamera;
    cam.mClipMin = 200.0f;
    cam.mClipMax = 1000000.0f;
  }

  glm::mat4 projMtx, viewMtx;
  mRenderSettings.mCameraController.mCamera.calcMatrices(width, height, projMtx,
                                                         viewMtx);

  mSceneState.invalidate();

  if (disp_opts.show_brres) {
    if (mVrcornModel) {
      mVrcornModel->addNodesToBuffer(mSceneState, viewMtx, projMtx);
    }
    if (mCourseModel) {
      mCourseModel->addNodesToBuffer(mSceneState, viewMtx, projMtx);
    }
  }

  if (mKmp != nullptr) {
    int i = 0;
    switch (mPage) {
    case Page::StartPoints:
      for (auto& pt : mKmp->mStartPoints) {
        glm::mat4 modelMtx = MatrixOfPoint(pt.position, pt.rotation, 1);
        PushCube(mSceneState, modelMtx, viewMtx, projMtx);

        SelectedPath path{.vector_addr = &mKmp->mStartPoints,
                          .index = (size_t)i++};
        if (isSelected(path.vector_addr, path.index)) {
          if (mSelectedObjectTransformEdit.owned_by == path) {
            if (mSelectedObjectTransformEdit.dirty) {
              auto [pos, rot, range] =
                  PointOfMatrix(mSelectedObjectTransformEdit.matrix);

              pt.position = pos;
              pt.rotation = rot;

              mSelectedObjectTransformEdit.dirty = false;
            }
          } else {
            mSelectedObjectTransformEdit = {
                .matrix = modelMtx, .owned_by = path, .dirty = false};
          }
        }
      }
      break;
    case Page::Objects:
      for (auto& pt : mKmp->mGeoObjs) {
        glm::mat4 modelMtx = MatrixOfPoint(pt.position, pt.rotation, 1);
        PushCube(mSceneState, modelMtx, viewMtx, projMtx);
      }
      break;
    case Page::Areas:
      for (auto& pt : mKmp->mAreas) {
        glm::mat4 modelMtx =
            MatrixOfPoint(pt.getModel().mPosition, pt.getModel().mRotation, 1);
        PushCube(mSceneState, modelMtx, viewMtx, projMtx);
      }
      break;
    case Page::Cameras:
      for (auto& pt : mKmp->mCameras) {
        glm::mat4 modelMtx = MatrixOfPoint(pt.mPosition, pt.mRotation, 1);
        PushCube(mSceneState, modelMtx, viewMtx, projMtx);
      }
      break;
    case Page::Respawns:
      for (auto& pt : mKmp->mRespawnPoints) {
        glm::mat4 modelMtx = MatrixOfPoint(pt.position, pt.rotation, pt.range);
        PushCube(mSceneState, modelMtx, viewMtx, projMtx);

        SelectedPath path{.vector_addr = &mKmp->mRespawnPoints,
                          .index = (size_t)i++};
        if (isSelected(path.vector_addr, path.index)) {
          if (mSelectedObjectTransformEdit.owned_by == path) {
            if (mSelectedObjectTransformEdit.dirty) {
              auto [pos, rot, range] =
                  PointOfMatrix(mSelectedObjectTransformEdit.matrix);

              pt.position = pos;
              pt.rotation = rot;

              mSelectedObjectTransformEdit.dirty = false;
            }
          } else {
            mSelectedObjectTransformEdit = {
                .matrix = modelMtx, .owned_by = path, .dirty = false};
          }
        }
      }
      break;
    case Page::Cannons:
      for (auto& pt : mKmp->mCannonPoints) {
        glm::mat4 modelMtx =
            MatrixOfPoint(pt.getPosition(), pt.getRotation(), 1);
        PushCube(mSceneState, modelMtx, viewMtx, projMtx);
      }
      break;
    case Page::MissionPoints:
      for (auto& pt : mKmp->mMissionPoints) {
        glm::mat4 modelMtx = MatrixOfPoint(pt.position, pt.rotation, 1);
        PushCube(mSceneState, modelMtx, viewMtx, projMtx);
      }
      break;
    }
  }

  if (disp_opts.show_kcl && mCourseKcl) {
    // Z sort
    if (disp_opts.xlu_mode == XluMode::Fancy) {
      mTriangleRenderer.sortTriangles(viewMtx);
    }

    mTriangleRenderer.draw(mSceneState, glm::mat4(1.0f), viewMtx, projMtx,
                           disp_opts.attr_mask.value, disp_opts.kcl_alpha);
  }

  if (disp_opts.show_map && mMapModel) {
    auto& opa = mSceneState.getBuffers().opaque.nodes;
    auto& xlu = mSceneState.getBuffers().translucent.nodes;
    // TODO: This assumes the BRRES adds on to the end
    const size_t begin_opa = opa.size();
    const size_t begin_xlu = xlu.size();

    ImGui::InputFloat("Minimap ScaleY", &mini_scale_y);

    mMapModel->addNodesToBuffer(
        mSceneState,
        viewMtx * glm::scale(glm::mat4(1.0f), glm::vec3(1, mini_scale_y, 1)),
        projMtx);

    // Transfer to XLU pass
    xlu.insert(xlu.begin() + begin_xlu, opa.begin() + begin_opa, opa.end());
    opa.resize(begin_opa);

    std::vector<librii::gfx::SceneNode*> map_nodes;
    for (size_t i = begin_xlu; i < xlu.size(); ++i)
      map_nodes.push_back(&xlu[i]);

    for (auto* node : map_nodes) {
      node->mega_state.fill = librii::gfx::PolygonMode::Line;
      node->mega_state.depthCompare = GL_ALWAYS;
      node->mega_state.depthWrite = GL_TRUE;
      node->mega_state.cullMode = -1;
    }
  }

  mSceneState.buildUniformBuffers();

  librii::glhelper::ClearGlScreen();
  mSceneState.draw();

  const bool view_updated =
      DrawViewCube(mViewport.mLastResolution.width,
                   mViewport.mLastResolution.height, viewMtx);
  auto& cam = mRenderSettings.mCameraController;
  if (view_updated)
    frontend::SetCameraControllerToMatrix(cam, viewMtx);

  static Manipulator manip;

  if (!mSelectedObjectTransformEdit.dirty) {
    manip.drawUi(mSelectedObjectTransformEdit.matrix);
    auto backup = mSelectedObjectTransformEdit.matrix;
    manip.manipulate(mSelectedObjectTransformEdit.matrix, viewMtx, projMtx);
    mSelectedObjectTransformEdit.dirty |=
        mSelectedObjectTransformEdit.matrix != backup;
  }

  if (mKmp != nullptr) {
    mKmpHistory.update(*mKmp);
  }
}

} // namespace riistudio::lvl
