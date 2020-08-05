#pragma once

#include <queue>
#include <string>

#include <frontend/applet.hpp>

#include "ThemeManager.hpp"
#include "file_host.hpp"

#include <frontend/editor/ImporterWindow.hpp>

namespace riistudio::frontend {

class EditorWindow;

class RootWindow final : public Applet, FileHost {
public:
  RootWindow();
  ~RootWindow();
  void draw() override;
  void onFileOpen(FileData data, OpenFilePolicy policy) override;

  void vdropDirect(std::unique_ptr<uint8_t[]> data, std::size_t len,
                   const std::string& name) override {
    FileHost::dropDirect(std::move(data), len, name);
  }
  void attachEditorWindow(std::unique_ptr<EditorWindow> editor);
  void save(const std::string& path);
  void saveAs();

private:
  u32 dockspace_id = 0;
  bool vsync = true;
  bool bDemo = false;
  float mFontGlobalScale = 1.0f;

  std::queue<std::string> mAttachEditorsQueue;
  ThemeManager mTheme;
  ThemeManager::BasicTheme mCurTheme = ThemeManager::BasicTheme::CorporateGrey;
  bool mThemeUpdated = true;

  bool mShowChangeLog = true;
  std::queue<ImporterWindow> mImportersQueue;
};

} // namespace riistudio::frontend
