#pragma once

#include <queue>
#include <string>

#include <frontend/applet.hpp>

#include "ThemeManager.hpp"
#include "file_host.hpp"

#include <frontend/UpdaterView.hpp>
#include <frontend/editor/ImporterWindow.hpp>
#include <frontend/widgets/theme_editor.hpp>

#include <rsl/Discord.hpp>

namespace riistudio::frontend {

class EditorWindow;

class DiscordRPCManager {
public:
  DiscordRPCManager() : mDiscordRpc("1074030676648669235") {}

  void connect() {
    mDiscordRpc.connect();
    mDiscordRpc.set_activity(mActivity);
  }

  void setStatus(std::string&& status) {
    if (mActivity.details != status) {
      mActivity.details = std::move(status);
      mDiscordRpc.set_activity(mActivity);
    }
  }

private:
  rsl::DiscordIpcClient mDiscordRpc;
  rsl::Activity mActivity{
      .state = "RiiStudio",
      .details = "Idling", // Filled in by editor
      .timestamps = {
        .start = time(0),
      },
      .assets = {.large_image = "large-image", .large_text = "Large text"},
      .buttons =
          {
              {
                  .text = "GitHub",
                  .link = "https://github.com/riidefi/RiiStudio",
              },
          },
  };
};

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
  void save(const std::string& path);
  void saveAs();

  enum class UnsavedProgressResult {
    Save,
    DontSave,
    CancelClose,
  };
  UnsavedProgressResult unsavedProgressBox();

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
