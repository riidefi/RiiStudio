#pragma once

#include <queue>
#include <string>

#include <frontend/applet.hpp>

#include "ThemeManager.hpp"
#include "file_host.hpp"

#include <frontend/UpdaterView.hpp>
#include <frontend/widgets/theme_editor.hpp>

#include <frontend/DiscordRPCManager.hpp>

namespace riistudio::frontend {

class RootWindow final : public Applet, public FileHost {
public:
  static RootWindow* spInstance;

  RootWindow();
  RootWindow(const RootWindow&) = delete;
  RootWindow(RootWindow&&) = delete;
  ~RootWindow();
  void draw() override;
  void drawStatusBar();
  void drawMenuBar();
  void drawLangMenu();
  void drawSettingsMenu();
  void drawFileMenu();
  void onFileOpen(FileData data, OpenFilePolicy policy) override;

  void vdropDirect(std::unique_ptr<uint8_t[]> data, std::size_t len,
                   const std::string& name) override {
    FileHost::dropDirect(std::move(data), len, name);
  }

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
