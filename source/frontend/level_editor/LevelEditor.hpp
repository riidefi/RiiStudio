#pragma once

#include <core/common.h>
#include <frontend/editor/StudioWindow.hpp>
#include <frontend/level_editor/Archive.hpp>
#include <frontend/level_editor/TriangleRenderer.hpp>
#include <frontend/renderer/Renderer.hpp>
#include <frontend/widgets/AutoHistory.hpp>
#include <frontend/widgets/DeltaTime.hpp>
#include <librii/g3d/gfx/G3dGfx.hpp>
#include <librii/kcol/Model.hpp>
#include <librii/kmp/CourseMap.hpp>
#include <plate/toolkit/Viewport.hpp>
#include <plugins/g3d/collection.hpp>

namespace riistudio::lvl {

enum class XluMode { Fast, Fancy };

struct AttributesBitmask {
  u32 value = 0;

  void enable(int i) { value |= 1 << i; }
  int num_enabled() const { return std::popcount(value); }

  bool is_enabled(int i) const { return value & (1 << i); }

  // value |= get_flag(i) -> is_enabled(i) == true
  u32 get_flag(int i) const { return 1 << i; }
};

struct RenderOptions {
  bool show_brres = true;
  bool show_kcl = true;
  bool show_map = true;
  AttributesBitmask attr_mask{
      .value = 0xc216f000 | 0xffff'ffff // KCL Flags to keep, default to walls
  };
  AttributesBitmask target_attr_all{
      .value = 0xffff'ffff // All flags in the scene
  };
  // Fancy -> enable per-triangle z-sorting
  XluMode xlu_mode{
#ifdef BUILD_DEBUG
      XluMode::Fast // the std::sort is incredibly slow on debug builds
#else
      XluMode::Fancy
#endif
  };

  float kcl_alpha = 0.5f;

  void init(librii::kcol::KCollisionData& kcl) {
    for (const auto& prism : kcl.prism_data) {
      target_attr_all.enable(prism.attribute & 31);
    }
  }
};

struct Level {
  // This is the original SZS file
  Archive root_archive;
  // Original path of .szs
  std::string og_path;
};

struct SelectedPath {
  void* vector_addr = nullptr;
  size_t index = 0;

  bool operator==(const SelectedPath&) const = default;
  bool operator<(const SelectedPath& rhs) const {
    return vector_addr < rhs.vector_addr || index < rhs.index;
  }
};

struct Selection {
  std::set<SelectedPath> selection;

  bool isSelected(void* vector_addr, size_t index) const {
    auto path = SelectedPath{.vector_addr = vector_addr, .index = index};

    return selection.contains(path);
  }
  void select(void* vector_addr, size_t index) {
    auto path = SelectedPath{.vector_addr = vector_addr, .index = index};

    selection.insert(path);
  }
  void deselect(void* vector_addr, size_t index) {
    auto path = SelectedPath{.vector_addr = vector_addr, .index = index};

    selection.erase(path);
  }
  void setSelected(void* vector_addr, size_t index, bool selected) {
    if (selected)
      select(vector_addr, index);
    else
      deselect(vector_addr, index);
  }
};

class KmpHistory {
public:
  void init(librii::kmp::CourseMap& map) { mLedger.update(map); }
  void update(librii::kmp::CourseMap& map) { mLedger.update(map); }

private:
  AutoHistory<librii::kmp::CourseMap> mLedger;
};

struct RenderableBRRES {
  RenderableBRRES(std::unique_ptr<g3d::Collection>&& collection)
      : mCollection(std::move(collection)) {
    assert(mCollection != nullptr);

    invalidate();
  }

  // Append draw calls to a buffer
  void addNodesToBuffer(riistudio::lib3d::SceneState& state, glm::mat4 v_mtx,
                        glm::mat4 p_mtx) {
    assert(mCollection != nullptr);
    assert(mRenderData != nullptr);
    librii::g3d::gfx::G3DSceneAddNodesToBuffer(state, *mCollection, v_mtx,
                                               p_mtx, *mRenderData);
  }

  // Flush the cache and update the render data
  void invalidate() {
    mRenderData = librii::g3d::gfx::G3DSceneCreateRenderData(*mCollection);
    assert(mRenderData != nullptr);
  }

  std::unique_ptr<g3d::Collection> mCollection;
  std::unique_ptr<librii::g3d::gfx::G3dSceneRenderData> mRenderData;
};

class LevelEditorWindow : public frontend::StudioWindow, private Selection {
public:
  LevelEditorWindow()
      : StudioWindow("Level Editor: <unknown>", frontend::DockSetting::Dockspace) {
    setWindowFlag(ImGuiWindowFlags_MenuBar);

    mRenderSettings.mCameraController.combo_choice_cam =
        frontend::CameraController::ControllerType::WASD_Plane;
  }

  void openFile(std::span<const u8> buf, std::string path);
  void saveFile(std::string path);

  void draw_() override;
  ImGuiID buildDock(ImGuiID root_id) override;

  void drawScene(u32 width, u32 height);

  void DrawRespawnTable();
  void DrawStartPointTable();
  void DrawAreaTable();
  void DrawCameraTable();
  void DrawCannonTable();
  void DrawMissionTable();
  void DrawStages();

  void DrawEnemyPathTable();
  void DrawEnemyPointTable(librii::kmp::EnemyPath& path);

  void DrawItemPathTable();
  void DrawItemPointTable(librii::kmp::ItemPath& path);

  void DrawCheckPathTable();
  void DrawCheckPointTable(librii::kmp::CheckPath& path);

  glm::mat4 getFrame(int frame);

  Level mLevel;
  plate::tk::Viewport mViewport;
  frontend::RenderSettings mRenderSettings;
  frontend::MouseHider mMouseHider;

  DeltaTimer mDeltaTimer;
  lib3d::SceneState mSceneState;

  std::unique_ptr<RenderableBRRES> mCourseModel;
  std::unique_ptr<RenderableBRRES> mVrcornModel;
  std::unique_ptr<RenderableBRRES> mMapModel;
  float mini_scale_y = 1.0f;
  std::unique_ptr<librii::kcol::KCollisionData> mCourseKcl;
  TriangleRenderer mTriangleRenderer;

  std::unique_ptr<librii::kmp::CourseMap> mKmp;
  KmpHistory mKmpHistory;

  RenderOptions disp_opts;

  enum class Page {
    StartPoints,
    EnemyPaths,
    EnemyPaths_Sub,
    ItemPaths,
    ItemPaths_Sub,
    CheckPaths,
    CheckPaths_Sub,
    Objects,
    Paths,
    Areas,
    Cameras,
    Respawns,
    Cannons,
    MissionPoints,
    Stages
  };
  Page mPage = Page::Respawns;
  int mSubPageID = 0; // For EnemyPaths_Sub

  // Currently, only one manipulator is supported
  struct SelectedObjectTransformEdit {
    glm::mat4 matrix;
    SelectedPath owned_by;
    bool dirty = false; // OR'd if matrix changed. Can only be unset by other
                        // code.
                        // Manipulator will not be drawn until acknowledged.

    bool isOwnedBy(const SelectedPath& owner) const {
      return owned_by == owner;
    }
  };
  SelectedObjectTransformEdit mSelectedObjectTransformEdit;

  std::string mErrDisp;

  // For tables
  bool show_coords = false;

  bool mDrawDebug = true;

  template <typename T> void AddNewToList(T& t) { t.emplace_back(); }
  template <typename T>
  void TryDeleteFromListAndUpdateSelection(T& t, int delete_index) {
    if (delete_index >= 0) {
      if (isSelected(&t, delete_index))
        selection.clear();
      t.erase(t.begin() + delete_index);
    }
  }
};

} // namespace riistudio::lvl
