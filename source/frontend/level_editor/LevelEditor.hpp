#pragma once

#include <core/common.h>
#include <filesystem>
#include <frontend/editor/StudioWindow.hpp>
#include <frontend/renderer/Renderer.hpp>
#include <librii/u8/U8.hpp>
#include <map>
#include <memory>
#include <plate/toolkit/Viewport.hpp>
#include <plugins/g3d/collection.hpp>
#include <span>
#include <string>

namespace riistudio::lvl {

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

class LevelEditorWindow : public frontend::StudioWindow {
public:
  LevelEditorWindow() : StudioWindow("Level Editor", true) {}

  void openFile(std::span<const u8> buf, std::string path);

  void draw_() override;

  void drawScene(u32 width, u32 height);

  Level mLevel;
  plate::tk::Viewport mViewport;
  frontend::RenderSettings mRenderSettings;
  frontend::MouseHider mMouseHider;
  lib3d::SceneState mSceneState;

  std::unique_ptr<g3d::Collection> mCourseModel;
  std::unique_ptr<g3d::Collection> mVrcornModel;
};

} // namespace riistudio::lvl
