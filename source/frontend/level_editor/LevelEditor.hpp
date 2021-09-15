#pragma once

#include <core/common.h>
#include <frontend/editor/StudioWindow.hpp>
#include <librii/u8/U8.hpp>
#include <map>
#include <memory>
#include <span>
#include <string>

namespace riistudio::lvl {

struct Archive {
  std::map<std::string, std::shared_ptr<Archive>> folders;
  std::map<std::string, std::vector<u8>> files;
};

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

  Level mLevel;
};

} // namespace riistudio::lvl
