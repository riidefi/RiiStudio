#pragma once

#include <core/common.h>
#include <filesystem>
#include <frontend/editor/StudioWindow.hpp>
#include <frontend/renderer/Renderer.hpp>
#include <librii/kcol/Model.hpp>
#include <librii/kmp/CourseMap.hpp>
#include <librii/u8/U8.hpp>
#include <map>
#include <memory>
#include <plate/toolkit/Viewport.hpp>
#include <plugins/g3d/collection.hpp>
#include <span>
#include <string>

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

struct Archive {
  std::map<std::string, std::shared_ptr<Archive>> folders;
  std::map<std::string, std::vector<u8>> files;
};

inline std::optional<std::vector<u8>> FindFile(Archive& arc, std::string path) {
  std::filesystem::path _path = path;
  _path = _path.lexically_normal();

  Archive* cur_arc = &arc;
  for (auto& part : _path) {
    {
      auto it = cur_arc->folders.find(part.string());
      if (it != cur_arc->folders.end()) {
        cur_arc = it->second.get();
        continue;
      }
    }
    {
      auto it = cur_arc->files.find(part.string());
      if (it != cur_arc->files.end()) {
        // TODO: This will ignore everything else in the path and accept invalid
        // item e.g. source/file.txt/invalid/other would ignore invalid/other
        return it->second;
      }
    }
  }

  return std::nullopt;
}

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
};

} // namespace riistudio::lvl
