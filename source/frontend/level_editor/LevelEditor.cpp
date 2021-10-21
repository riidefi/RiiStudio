#include "LevelEditor.hpp"
#include "IO.hpp"
#include "KclUtil.hpp"
#include "ObjUtil.hpp"
#include "Transform.hpp"
#include <bit>
#include <core/3d/gl.hpp>
#include <core/common.h>
#include <core/util/gui.hpp>
#include <core/util/oishii.hpp>
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

struct ResolveQuery {
  std::vector<u8> file_data;
  std::string resolved_path;
};

std::optional<ResolveQuery>
FindFileWithOverloads(const Archive& arc, std::vector<std::string> paths) {
  for (auto& path : paths) {
    auto found = FindFile(arc, path);
    if (found.has_value()) {
      return ResolveQuery{.file_data = std::move(*found),
                          .resolved_path = path};
    }
  }

  return std::nullopt;
}

void LevelEditorWindow::openFile(std::span<const u8> buf, std::string path) {
  std::string errc;
  auto root_arc = ReadArchive(buf, errc);
  if (!root_arc.has_value()) {
    mErrDisp = errc;
    return;
  }

  mLevel.root_archive = std::move(*root_arc);
  mLevel.og_path = path;

  auto course_model_brres = FindFileWithOverloads(
      mLevel.root_archive, {"course_d_model.brres", "course_model.brres"});
  if (course_model_brres.has_value()) {
    mCourseModel = ReadBRRES(course_model_brres->file_data,
                             course_model_brres->resolved_path);
  }

  auto vrcorn_model_brres = FindFileWithOverloads(
      mLevel.root_archive, {"vrcorn_d_model.brres", "vrcorn_model.brres"});
  if (course_model_brres.has_value()) {
    mVrcornModel = ReadBRRES(vrcorn_model_brres->file_data,
                             vrcorn_model_brres->resolved_path);
  }

  auto map_model = FindFile(mLevel.root_archive, "map_model.brres");
  if (map_model.has_value()) {
    mMapModel = ReadBRRES(*map_model, "map_model.brres");
  }

  auto course_kcl = FindFile(mLevel.root_archive, "course.kcl");
  if (course_kcl.has_value()) {
    mCourseKcl = ReadKCL(*course_kcl, "course.kcl");
  }

  if (mCourseKcl) {
    mTriangleRenderer.init(*mCourseKcl);

    for (auto& prism : mCourseKcl->prism_data) {
      disp_opts.target_attr_all.enable(prism.attribute & 31);
    }
  }

  auto course_kmp = FindFile(mLevel.root_archive, "course.kmp");
  if (course_kmp.has_value()) {
    mKmp = ReadKMP(*course_kmp, "course.kmp");

    mKmpHistory.update(*mKmp);
  }

  auto& cam = mRenderSettings.mCameraController.mCamera;
  cam.mClipMin = 200.0f;
  cam.mClipMax = 1000000.0f;
  mRenderSettings.mCameraController.mSpeed = 15'000.0f;

  if (mKmp && !mKmp->mStartPoints.empty()) {
    auto& start = mKmp->mStartPoints[0];
    cam.mEye = start.position + glm::vec3(0.0f, 10'000.0f, 0.0f);
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

struct Property {
  std::string name;

  rsl::RNA type;
  bool is_mutable = true;

  size_t offset;
  size_t size;
};

#define FLOAT_P(Name, Mem)                                                     \
  Property {                                                                   \
    .name = Name,                                                              \
    .type = rsl::to_rna<decltype(librii::kmp::RespawnPoint::Mem)>,             \
    .is_mutable = true, .offset = offsetof(librii::kmp::RespawnPoint, Mem),    \
    .size = sizeof(librii::kmp::RespawnPoint::Mem)                             \
  }

std::array<Property, 8> respawn_properties = {
    FLOAT_P("Position.x", position.x), FLOAT_P("Position.y", position.y),
    FLOAT_P("Position.z", position.z), FLOAT_P("Rotation.x", rotation.x),
    FLOAT_P("Rotation.y", rotation.y), FLOAT_P("Rotation.z", rotation.z),
    FLOAT_P("Radius", range),          FLOAT_P("ID", id),
};

#undef FLOAT_P

std::string to_json(const librii::kmp::RespawnPoint& r) {
  std::string result = "{\n";
  for (int i = 0; i < respawn_properties.size(); ++i) {
    const auto& p = respawn_properties[i];
    result += "  \"";
    result += p.name;
    result += "\": ";
    result += std::to_string(
        rsl::ReadAny(rsl::BytesOf(r).subspan(p.offset), p.type).value);
    if (i + 1 != respawn_properties.size())
      result += ",\n";
  }

  return result + "\n}";
}

void LevelEditorWindow::DrawRespawnTable() {
  if (selection.size()) {
    auto j = to_json(mKmp->mRespawnPoints[selection.begin()->index]);
    ImGui::Text(j.c_str());
  }
  auto flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings |
               ImGuiTableFlags_RowBg;
  if (ImGui::BeginTable("##TBL2", respawn_properties.size() + 2, flags)) {
    ImGui::TableSetupColumn("Selected");
    ImGui::TableSetupColumn("Index");
    for (auto& p : respawn_properties)
      ImGui::TableSetupColumn(p.name.c_str());
    ImGui::TableAutoHeaders();

    for (int i = 0; i < mKmp->mRespawnPoints.size(); ++i) {
      auto& pt = mKmp->mRespawnPoints[i];
      util::IDScope g(i);

      ImGui::TableNextRow();
      ImGui::PushStyleColor(ImGuiCol_FrameBg, 0);

      bool selected = isSelected(&mKmp->mRespawnPoints, i);
      if (ImGui::Checkbox("##Selected", &selected)) {
        selection.clear();
        setSelected(&mKmp->mRespawnPoints, i, selected);
      }
      ImGui::TableNextCell();

      ImGui::Text("%i", i);
      ImGui::TableNextCell();

      for (auto& p : respawn_properties) {
        auto bytes = rsl::BytesOf(pt).subspan(p.offset);
        auto any = rsl::ReadAny(bytes, p.type);

        ImGui::InputDouble(p.name.c_str(), &any.value);

        rsl::WriteAny(bytes, any);

        ImGui::TableNextCell();
      }

      ImGui::PopStyleColor();
    }
    ImGui::EndTable();
  }
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
  if (ImGui::Begin("Hi")) {
    auto clicked = GatherNodes(mLevel.root_archive);
    if (clicked.has_value()) {
      auto* pParent = frontend::RootWindow::spInstance;
      if (pParent) {
        pParent->dropDirect(clicked->second, clicked->first);
      }
    }
  }
  ImGui::End();

  if (ImGui::Begin("View", nullptr, ImGuiWindowFlags_MenuBar)) {
    auto bounds = ImGui::GetWindowSize();
    if (mViewport.begin(static_cast<u32>(bounds.x),
                        static_cast<u32>(bounds.y))) {
      drawScene(bounds.x, bounds.y);

      mViewport.end();
    }
  }
  ImGui::End();

  if (ImGui::Begin("Tree", nullptr, ImGuiWindowFlags_MenuBar)) {
    static ImGuiTableFlags flags =
        ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersHOuter |
        ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg;
    // ImGui::CheckboxFlags("ImGuiTableFlags_Scroll", (unsigned int*)&flags,
    // ImGuiTableFlags_Scroll);
    // ImGui::CheckboxFlags("ImGuiTableFlags_ScrollFreezeLeftColumn", (unsigned
    // int*)&flags, ImGuiTableFlags_ScrollFreezeLeftColumn);

    if (ImGui::BeginTable("##3ways", 2, flags)) {
      // The first column will use the default _WidthStretch when ScrollX is Off
      // and _WidthFixed when ScrollX is On
      ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
      ImGui::TableSetupColumn("Children", ImGuiTableColumnFlags_WidthFixed,
                              ImGui::GetFontSize() * 6);
      // ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed,
      //                        ImGui::GetFontSize() * 10);
      ImGui::TableAutoHeaders();

      struct X {
        static bool BeginCustomSelectable(bool sel) {
          if (!sel) {
            ImGui::Text("( )");
            ImGui::SameLine();
            return false;
          }

          ImGui::Text("(X)");
          ImGui::SameLine();
          ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{1.0f, 1.0f, 0.0f, 1.0f});
          return true;
        }
        static void EndCustomSelectable(bool sel) {
          if (!sel)
            return;

          ImGui::PopStyleColor();
        }

        struct Defer {
          Defer(std::function<void()> f) : mF(f) {}
          Defer(Defer&& rhs) : mF(rhs.mF) { rhs.mF = nullptr; }
          ~Defer() {
            if (mF)
              mF();
          }

          std::function<void()> mF;
        };

        static Defer RAIICustomSelectable(bool sel) {
          bool b = X::BeginCustomSelectable(sel);
          return Defer{[b] { X::EndCustomSelectable(b); }};
        }

        static bool IsNodeSwitchedTo() {
          return ImGui::IsItemClicked() || ImGui::IsItemFocused();
        }

        // Seems we don't need to care about the tree_node_ex_result
        // static bool IsNodeSwitchedTo(bool tree_node_ex_result) {
        //   return tree_node_ex_result && IsNodeSwitchedTo();
        // }
      };

      {
        ImGui::TableNextRow();

        auto d = X::RAIICustomSelectable(mPage == Page::StartPoints);

        if (ImGui::TreeNodeEx("Start Points", ImGuiTreeNodeFlags_Leaf)) {
          ImGui::TreePop();
        }
        if (X::IsNodeSwitchedTo()) {
          mPage = Page::StartPoints;
        }
        ImGui::TableNextCell();
        ImGui::Text("%i", static_cast<int>(mKmp->mStartPoints.size()));
      }

      {
        bool has_child = false;
        {
          ImGui::TableNextRow();
          auto d = X::RAIICustomSelectable(mPage == Page::EnemyPaths);

          has_child = ImGui::TreeNodeEx("Enemy Paths", 0);
          if (X::IsNodeSwitchedTo()) {
            mPage = Page::EnemyPaths;
          }

          ImGui::TableNextCell();
          ImGui::Text("%i", static_cast<int>(mKmp->mEnemyPaths.size()));
        }

        if (has_child) {

          for (int i = 0; i < mKmp->mEnemyPaths.size(); ++i) {
            ImGui::TableNextRow();

            auto d = X::RAIICustomSelectable(mPage == Page::EnemyPaths_Sub &&
                                             mSubPageID == i);

            std::string node_s = "Enemy Path #" + std::to_string(i);
            if (ImGui::TreeNodeEx(node_s.c_str(), ImGuiTreeNodeFlags_Leaf)) {
              ImGui::TreePop();
            }

            if (X::IsNodeSwitchedTo()) {
              mPage = Page::EnemyPaths_Sub;
              mSubPageID = i;
            }
          }

          ImGui::TreePop();
        }
      }

      {
        ImGui::TableNextRow();

        bool b = X::BeginCustomSelectable(mPage == Page::ItemPaths);
        if (ImGui::TreeNodeEx("Item Paths", ImGuiTreeNodeFlags_Leaf)) {
          if (X::IsNodeSwitchedTo()) {
            mPage = Page::ItemPaths;
          }
          ImGui::TreePop();
        }
        ImGui::TableNextCell();
        ImGui::Text("%i", static_cast<int>(mKmp->mItemPaths.size()));
        X::EndCustomSelectable(b);
      }

      {
        ImGui::TableNextRow();

        bool b = X::BeginCustomSelectable(mPage == Page::CheckPaths);
        if (ImGui::TreeNodeEx("Checkpoints", ImGuiTreeNodeFlags_Leaf)) {
          if (X::IsNodeSwitchedTo()) {
            mPage = Page::CheckPaths;
          }
          ImGui::TreePop();
        }
        ImGui::TableNextCell();
        ImGui::Text("%i", static_cast<int>(mKmp->mCheckPaths.size()));
        X::EndCustomSelectable(b);
      }

      {
        ImGui::TableNextRow();

        bool b = X::BeginCustomSelectable(mPage == Page::Objects);
        if (ImGui::TreeNodeEx("Objects", ImGuiTreeNodeFlags_Leaf)) {
          if (X::IsNodeSwitchedTo()) {
            mPage = Page::Objects;
          }
          ImGui::TreePop();
        }
        ImGui::TableNextCell();
        ImGui::Text("%i", static_cast<int>(mKmp->mGeoObjs.size()));
        X::EndCustomSelectable(b);
      }

      {
        ImGui::TableNextRow();

        bool b = X::BeginCustomSelectable(mPage == Page::Paths);
        if (ImGui::TreeNodeEx("Paths", ImGuiTreeNodeFlags_Leaf)) {
          if (X::IsNodeSwitchedTo()) {
            mPage = Page::Paths;
          }
          ImGui::TreePop();
        }
        ImGui::TableNextCell();
        ImGui::Text("%i", static_cast<int>(mKmp->mPaths.size()));
        X::EndCustomSelectable(b);
      }

      {
        ImGui::TableNextRow();

        bool b = X::BeginCustomSelectable(mPage == Page::Areas);
        if (ImGui::TreeNodeEx("Areas", ImGuiTreeNodeFlags_Leaf)) {
          if (X::IsNodeSwitchedTo()) {
            mPage = Page::Areas;
          }
          ImGui::TreePop();
        }
        ImGui::TableNextCell();
        ImGui::Text("%i", static_cast<int>(mKmp->mAreas.size()));
        X::EndCustomSelectable(b);
      }

      {
        ImGui::TableNextRow();

        bool b = X::BeginCustomSelectable(mPage == Page::Cameras);
        if (ImGui::TreeNodeEx("Cameras", ImGuiTreeNodeFlags_Leaf)) {
          if (X::IsNodeSwitchedTo()) {
            mPage = Page::Cameras;
          }
          ImGui::TreePop();
        }
        ImGui::TableNextCell();
        ImGui::Text("%i", static_cast<int>(mKmp->mCameras.size()));
        X::EndCustomSelectable(b);
      }

      {
        ImGui::TableNextRow();

        bool b = X::BeginCustomSelectable(mPage == Page::Respawns);
        if (ImGui::TreeNodeEx("Respawns", ImGuiTreeNodeFlags_Leaf)) {
          if (X::IsNodeSwitchedTo()) {
            mPage = Page::Respawns;
          }
          ImGui::TreePop();
        }
        ImGui::TableNextCell();
        ImGui::Text("%i", static_cast<int>(mKmp->mRespawnPoints.size()));
        X::EndCustomSelectable(b);
      }

      {
        ImGui::TableNextRow();

        bool b = X::BeginCustomSelectable(mPage == Page::Cannons);
        if (ImGui::TreeNodeEx("Cannons", ImGuiTreeNodeFlags_Leaf)) {
          if (X::IsNodeSwitchedTo()) {
            mPage = Page::Cannons;
          }
          ImGui::TreePop();
        }
        ImGui::TableNextCell();
        ImGui::Text("%i", static_cast<int>(mKmp->mCannonPoints.size()));
        X::EndCustomSelectable(b);
      }

      {
        ImGui::TableNextRow();

        bool b = X::BeginCustomSelectable(mPage == Page::MissionPoints);
        if (ImGui::TreeNodeEx("Mission Points", ImGuiTreeNodeFlags_Leaf)) {
          if (X::IsNodeSwitchedTo()) {
            mPage = Page::MissionPoints;
          }
          ImGui::TreePop();
        }
        ImGui::TableNextCell();
        ImGui::Text("%i", static_cast<int>(mKmp->mMissionPoints.size()));
        X::EndCustomSelectable(b);
      }

      {
        ImGui::TableNextRow();

        bool b = X::BeginCustomSelectable(mPage == Page::Stages);
        if (ImGui::TreeNodeEx("Stages", ImGuiTreeNodeFlags_Leaf)) {
          if (X::IsNodeSwitchedTo()) {
            mPage = Page::Stages;
          }
          ImGui::TreePop();
        }
        ImGui::TableNextCell();
        ImGui::Text("%i", static_cast<int>(mKmp->mStages.size()));
        X::EndCustomSelectable(b);
      }

      ImGui::TableNextRow();

      ImGui::EndTable();
    }
  }
  ImGui::End();

  if (ImGui::Begin("Properties", nullptr, ImGuiWindowFlags_MenuBar)) {
    switch (mPage) {
    case Page::StartPoints:
      break;
    case Page::Respawns:
      DrawRespawnTable();
      break;
    }
  }
  ImGui::End();
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

  cube.glBeginMode = GL_TRIANGLES;
  cube.vertex_count = librii::glhelper::GetCubeNumVerts();
  cube.glVertexDataType = GL_UNSIGNED_INT;
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
    ImGui::InputFloat3("Tr", matrixTranslation, 3);
    ImGui::InputFloat3("Rt", matrixRotation, 3);
    ImGui::InputFloat3("Sc", matrixScale, 3);
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
    ImGui::Checkbox("", &useSnap);
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
  mRenderSettings.drawMenuBar();

  // if (ImGui::BeginMenuBar()) {
  DrawRenderOptions(disp_opts);
  //   ImGui::EndMenuBar();
  // }

  if (!mRenderSettings.rend)
    return;

  librii::glhelper::SetGlWireframe(mRenderSettings.wireframe);

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
      mVrcornModel->prepare(mSceneState, *mVrcornModel, viewMtx, projMtx);
    }
    if (mCourseModel) {
      mCourseModel->prepare(mSceneState, *mCourseModel, viewMtx, projMtx);
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

  if (disp_opts.show_kcl) {
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

    mMapModel->prepare(
        mSceneState, *mMapModel,
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

  auto pos = ImGui::GetCursorScreenPos();
  // auto win = ImGui::GetWindowPos();
  auto avail = ImGui::GetContentRegionAvail();

  // int shifted_x = mViewport.mLastWidth - avail.x;
  int shifted_y = mViewport.mLastHeight - avail.y;

  // TODO: LastWidth is moved to the right, not left -- bug?
  ImGuizmo::SetRect(pos.x, pos.y - shifted_y, mViewport.mLastWidth,
                    mViewport.mLastHeight);

  auto tVm = viewMtx;
  auto max = ImGui::GetContentRegionMaxAbs();

  const u32 viewcube_bg = 0;
  // ImGui::GetColorU32(ImGui::GetStyle().Colors[ImGuiCol_WindowBg])

  ImGuizmo::ViewManipulate(glm::value_ptr(viewMtx), 20.0f,
                           {max.x - 200.0f, max.y - 200.0f}, {200.0f, 200.0f},
                           viewcube_bg);

  auto& cam = mRenderSettings.mCameraController;
  if (tVm != viewMtx)
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
