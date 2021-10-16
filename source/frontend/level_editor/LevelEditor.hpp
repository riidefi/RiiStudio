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

namespace riistudio::lvl {

enum class XluMode { Fast, Fancy };

struct RenderOptions {
  bool show_brres = true;
  bool show_kcl = true;
  u32 attr_mask = 0xc216f000; // KCL Flags to keep, default to walls
  u32 target_attr_all = -1;   // All flags in the scene

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

inline void CommitHistory(size_t& cursor,
                          std::vector<librii::kmp::CourseMap>& chain) {
  chain.resize(cursor + 1);
  cursor++;
}
inline void UndoHistory(size_t& cursor,
                        std::vector<librii::kmp::CourseMap>& chain) {
  if (cursor <= 0)
    return;
  --cursor;
}
inline void RedoHistory(size_t& cursor,
                        std::vector<librii::kmp::CourseMap>& chain) {
  if (cursor >= chain.size())
    return;
  ++cursor;
}

class LevelEditorWindow : public frontend::StudioWindow {
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

  struct Triangle {
    u16 attr;
    std::array<glm::vec3, 3> verts;
  };
  std::vector<Triangle> mKclTris;

  std::unique_ptr<librii::kmp::CourseMap> mKmp;
  std::vector<librii::kmp::CourseMap> mKmpHistory;
  size_t history_cursor = 0;
  bool commit_posted = false;

  RenderOptions disp_opts;

  // KCL triangle vertex buffer
  std::unique_ptr<librii::glhelper::VBOBuilder> tri_vbo = nullptr;
  // z_dist of every kcl triangle and the camera
  std::vector<float> z_dist;

  struct SelectedPath {
    void* vector_addr = nullptr;
    size_t index = 0;

    bool operator==(const SelectedPath&) const = default;
    bool operator<(const SelectedPath& rhs) const {
      return vector_addr < rhs.vector_addr || index < rhs.index;
    }
  };

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

  enum class Page {
    StartPoints,
    EnemyPaths,
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
};

} // namespace riistudio::lvl
