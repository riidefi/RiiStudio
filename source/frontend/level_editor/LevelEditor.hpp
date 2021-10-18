#pragma once

#include <core/common.h>
#include <frontend/editor/StudioWindow.hpp>
#include <frontend/renderer/Renderer.hpp>
#include <librii/kcol/Model.hpp>
#include <librii/kmp/CourseMap.hpp>
#include <memory>
#include <plate/toolkit/Viewport.hpp>
#include <plugins/g3d/collection.hpp>
#include <span>
#include <string>

#include <frontend/level_editor/Archive.hpp>
#include <frontend/level_editor/AutoHistory.hpp>
#include <frontend/level_editor/TriangleRenderer.hpp>

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
  AttributesBitmask attr_mask{
      .value = 0xc216f000 // KCL Flags to keep, default to walls
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

class LevelEditorWindow : public frontend::StudioWindow, private Selection {
public:
  LevelEditorWindow() : StudioWindow("Level Editor", true) {
    setWindowFlag(ImGuiWindowFlags_MenuBar);
  }

  void openFile(std::span<const u8> buf, std::string path);
  void saveFile(std::string path);

  void draw_() override;

  void drawScene(u32 width, u32 height);

  void DrawRespawnTable();

  Level mLevel;
  plate::tk::Viewport mViewport;
  frontend::RenderSettings mRenderSettings;
  frontend::MouseHider mMouseHider;
  lib3d::SceneState mSceneState;

  std::unique_ptr<g3d::Collection> mCourseModel;
  std::unique_ptr<g3d::Collection> mVrcornModel;
  std::unique_ptr<librii::kcol::KCollisionData> mCourseKcl;
  TriangleRenderer mTriangleRenderer;

  std::unique_ptr<librii::kmp::CourseMap> mKmp;
  AutoHistory<librii::kmp::CourseMap> mKmpHistory;

  RenderOptions disp_opts;

  enum class Page {
    StartPoints,
    EnemyPaths,
    EnemyPaths_Sub,
    ItemPaths,
    CheckPaths,
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
};

} // namespace riistudio::lvl
