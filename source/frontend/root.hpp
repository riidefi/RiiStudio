#pragma once

#include <queue>
#include <string>

#include <frontend/applet.hpp>

#include "ThemeManager.hpp"
#include "file_host.hpp"

#include <frontend/UpdaterView.hpp>
#include <frontend/editor/ImporterWindow.hpp>
#include <frontend/widgets/theme_editor.hpp>

#include <frontend/DiscordRPCManager.hpp>

namespace riistudio::frontend {

class EditorWindow;

class RootWindow final : public Applet, public FileHost {
public:
  static RootWindow* spInstance;

  RootWindow();
  RootWindow(const RootWindow&) = delete;
  RootWindow(RootWindow&&) = delete;
  ~RootWindow();
  void draw() override;
  void drawStatusBar();
  void processImportersQueue();
  void drawMenuBar(riistudio::frontend::EditorWindow* ed);
  void drawLangMenu();
  void drawSettingsMenu();
  void drawFileMenu(riistudio::frontend::EditorWindow* ed);
  void onFileOpen(FileData data, OpenFilePolicy policy) override;

  void vdropDirect(std::unique_ptr<uint8_t[]> data, std::size_t len,
                   const std::string& name) override {
    FileHost::dropDirect(std::move(data), len, name);
  }
  void attachEditorWindow(std::unique_ptr<EditorWindow> editor);

  void saveButton();
  void saveAsButton();

private:
  bool shouldClose() override;

  bool vsync = true;
  bool bDemo = false;

  ThemeData mThemeData;

  // std::queue<std::string> mAttachEditorsQueue;
  ThemeManager mTheme;
  bool mThemeUpdated = true;

  std::queue<ImporterWindow> mImportersQueue;

  // Hack...
  bool mWantFile = false;
  bool mGotFile = false;
  FileData mReqData;

  UpdaterView mUpdater;
  bool mCheckUpdate = true;

  DiscordRPCManager mDiscordRpc;

public:
  void requestFile() { mWantFile = true; }
  FileData* getFile() {
    if (!mGotFile)
      return nullptr;
    return &mReqData;
  }
  void endRequestFile() { mWantFile = false; }

  void setForceUpdate(bool update) { mUpdater.SetForceUpdate(update); }
  void setCheckUpdate(bool update) { mCheckUpdate = update; }
};

} // namespace riistudio::frontend
