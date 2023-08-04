#include <core/3d/gl.hpp>

#include "EditorFactory.hpp"
#include "IEditor.hpp"
#include "SettingsMenu.hpp"
#include "root.hpp"
#include <core/util/timestamp.hpp>
#include <frontend/Fonts.hpp>
#include <frontend/Localization.hpp>
#include <frontend/widgets/UnsavedProgress.hpp>
#include <frontend/widgets/fps.hpp>
#include <imcxx/Widgets.hpp>
#include <imgui_markdown.h>
#include <oishii/reader/binary_reader.hxx>
#include <oishii/writer/binary_writer.hxx>
#include <rsl/Discord.hpp>
#include <rsl/FsDialog.hpp>
#include <rsl/LeakDebug.hpp>
#include <rsl/Stb.hpp>

namespace llvm {
int EnableABIBreakingChecks;
int DisableABIBreakingChecks;
} // namespace llvm

bool gIsAdvancedMode = false;

bool IsAdvancedMode() { return gIsAdvancedMode; }

namespace libcube::UI {
void InstallCrate();
void ImageActionsInstaller();
} // namespace libcube::UI

namespace riistudio::frontend {

void DrawLocaleMenu() {
  static int locale = 0;

  int locale_opt = locale;
  ImGui::RadioButton("English", &locale_opt, 0);
  ImGui::RadioButton("日本語", &locale_opt, 1);

  if (locale_opt != locale) {
    locale = locale_opt;

    const char* id_str = "English";
    switch (locale) {
    default:
    case 0:
      id_str = "English";
      break;
    case 1:
      id_str = "Japanese";
      break;
    }

    riistudio::SetLocale(id_str);
  }

  if (ImGui::Button("Dump Untranslated Strings"_j)) {
    riistudio::DebugDumpLocaleMisses();
  }
  if (ImGui::Button("Reload Language .csv Files"_j)) {
    riistudio::DebugReloadLocales();
  }
}

RootWindow* RootWindow::spInstance;

void SetWindowIcon(void* platform_window, const char* path) {
#ifdef RII_BACKEND_GLFW
  if (platform_window == nullptr) {
    rsl::error("Failed to set icon: platform_window is NULL");
    return;
  }

  GLFWwindow* window = reinterpret_cast<GLFWwindow*>(platform_window);
  auto x = rsl::stb::load(path);
  if (!x.has_value()) {
    rsl::error("Failed to load icon");
    return;
  }

  rsl::info("Setting icon: width={},height={},data={}", x->width, x->height,
            reinterpret_cast<void*>(x->data.data()));
  GLFWimage image;
  image.width = x->width;
  image.height = x->height;
  image.pixels = x->data.data();
  glfwSetWindowIcon(window, 1, &image);
#endif
}

static void MSVCWarningWindow() {
#if !defined(HAS_RUST_TRY)
  ImGui::SetNextItemWidth(400);
  if (ImGui::Begin("Warning", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    auto back = ImGui::GetCurrentWindow()->FontWindowScale;
    ImGui::SetWindowFontScale(1.3f);
    util::Markdown(
        "Warning: This version of RiiStudio is built with Microsoft's Visual\n"
        "C++ Compiler (MSVC). MSVC does not support a C++ language extention\n"
        "called \"Statement Expressions\" that RiiStudio depends on. As such,\n"
        "error handling is severely hampered and some features may just not\n"
        "work. It's recommended you compile with the Clang compiler on\n"
        "Windows. In Visual Studio: Tools > Get Tools and Features. On the\n"
        "right-hand panel check \"C++ Clang tools for Windows\" and click\n"
        "\"Install while downloading.\" When installed, at the top bar switch\n"
        "the preset from msvc-x64-Debug to Clang-x64-debug (or clang-x64-DIST\n"
        "for a final build).");
    ImGui::SetWindowFontScale(back);
  }
  ImGui::End();
#endif // !defined(HAS_RUST_TRY)
}

void RootWindow::draw() {
  rsl::DoLeakCheck();
  auto on_file_open = [&](FileData x, OpenFilePolicy y) {
    onFileOpen(std::move(x), y);
  };
  mFileHost.fileHostProcess(on_file_open);

  if (auto* a = getActive()) {
    if (auto* b = dynamic_cast<IEditor*>(a)) {
      auto status = b->discordStatus();
      mDiscordRpc.setStatus(std::move(status));
    }
  }

  auto& io = ImGui::GetIO();
#ifdef RETINA_DEBUG
  ImGui::Text("DEBUG: Display: %f, %f", io.DisplaySize.x, io.DisplaySize.y);
  ImGui::Text("DEBUG: Apple Retina scaling: %f, %f",
              io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
#endif

  io.FontGlobalScale = mThemeData.mFontGlobalScale;

  if (mThemeUpdated) {
    // Reset theme; otherwise scale factors could compound later
    ImGui::GetStyle() = {};
    mTheme.setThemeEx(mThemeData.mTheme);
    mThemeUpdated = false;

    ImGui::GetStyle().ScaleAllSizes(mThemeData.mGlobalScale);
  }

  util::IDScope g(0);

  if (imcxx::BeginFullscreenWindow("##RootWindow", getOpen())) {
    drawStatusBar();

    const u32 dockspace_id = ImGui::GetID("DockSpaceWidget");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), 0);

    drawMenuBar();

    MSVCWarningWindow();

    if (bDemo)
      ImGui::ShowDemoWindow(&bDemo);

    if (mCheckUpdate && mUpdater.IsOnline())
      mUpdater.draw();

    drawChildren();

    if (mShowMkwDebug) {
      mMkwDebugWindow.draw();
    }
    if (mShowSettings) {
      DrawSettingsWindow();
    }
  }
  imcxx::EndFullscreenWindow();
}
void RootWindow::drawStatusBar() {
  ImGui::SetWindowFontScale(1.1f);
#ifndef NDEBUG
  {
    ImVec4 clr{1.0f, 0.0f, 0.0f, 1.0f};
    ImGui::TextColored(
        clr, "Warning: This is a Debug build with optimizations disabled");
  }
#endif
  if (!hasChildren()) {
    ImGui::TextUnformatted("Drop a file to edit."_j);
  }
  ImGui::SetWindowFontScale(1.0f);
}
void RootWindow::drawMenuBar() {
  // [File] [Settings] [Lang]   (---> WindowWidth-60) [FPS]
  if (ImGui::BeginMenuBar()) {
    drawFileMenu();

    drawSettingsMenu();

    if (ImGui::BeginMenu("Tools"_j)) {
      ImGui::Checkbox("Build Info"_j, &mShowSettings);
      ImGui::Checkbox("MKW Debug (Dolphin)"_j, &mShowMkwDebug);
      ImGui::EndMenu();
    }

    if (Fonts::IsJapaneseSupported())
      drawLangMenu();

    ImGui::SameLine(ImGui::GetWindowWidth() - 60);
    DrawFps();

    ImGui::EndMenuBar();
  }
}
void RootWindow::drawLangMenu() {
  if (ImGui::BeginMenu("日本語")) {
    DrawLocaleMenu();

    ImGui::EndMenu();
  }
}
void RootWindow::drawSettingsMenu() {
  if (ImGui::BeginMenu("Settings"_j)) {
    bool _vsync = vsync;
    ImGui::Checkbox("VSync"_j, &_vsync);

    if (_vsync != vsync) {
      setVsync(_vsync);
      vsync = _vsync;
    }

    mThemeUpdated |= DrawThemeEditor(mThemeData, nullptr);

#ifdef BUILD_DEBUG
    ImGui::Checkbox("ImGui Demo", &bDemo);
#endif

    ImGui::Checkbox("Advanced Mode"_j, &gIsAdvancedMode);

    ImGui::EndMenu();
  }
}
void RootWindow::drawFileMenu() {
  if (ImGui::BeginMenu("File"_j)) {
#if !defined(__EMSCRIPTEN__)
    if (ImGui::MenuItem("Open"_j)) {
      mFileHost.openFile();
    }
#endif
    if (ImGui::MenuItem("Save"_j)) {
      saveButton();
    }
#if !defined(__EMSCRIPTEN__)
    if (ImGui::MenuItem("Save As"_j)) {
      saveAsButton();
    }
#endif
    ImGui::EndMenu();
  }
}

void RootWindow::onFileOpen(FileData data, OpenFilePolicy policy) {
  rsl::info("Opening file: {}", data.mPath.c_str());

  auto w = MakeEditor(data);
  if (w) {
    attachWindow(std::move(w));
    return;
  }

  if (mWantFile) {
    mReqData = std::move(data);
    mGotFile = true;
    return;
  }
}

RootWindow::RootWindow()
    : Applet(std::string("RiiStudio "_j) + RII_TIME_STAMP) {
  spInstance = this;

  // Loads the plugins for file formats / importers
  libcube::UI::InstallCrate();
  libcube::UI::ImageActionsInstaller();

  // Without this, clicking in the viewport with a mouse would move the window
  // when undocked.
  ImGui::GetIO().ConfigWindowsMoveFromTitleBarOnly = true;

  SetWindowIcon(getPlatformWindow(), "icon.png");

  mDiscordRpc.connect();

  {
    auto brres = LoadLuigiCircuitSample();
    if (brres) {
      mFileHost.dropDirect(std::move(*brres), "./samples/luigi_circuit.brres");
    }
  }

#if 0
  // iOS has 3:1 DPI
  mThemeData.mGlobalScale = .534f;
  mThemeData.mFontGlobalScale = .534f;
#endif
}
RootWindow::~RootWindow() = default;

void RootWindow::saveButton() {
  if (getActive() == nullptr) {
    rsl::ErrorDialog("Failed to save. Nothing is open");
    return;
  }
  if (IEditor* b = dynamic_cast<IEditor*>(getActive())) {
    b->saveButton();
    return;
  }
  // In theory, you could handle custom types here implementing a different API.
  rsl::ErrorDialog("Failed to save. Not saveable");
}

void RootWindow::saveAsButton() {
  if (getActive() == nullptr) {
    rsl::ErrorDialog("Failed to save. Nothing is open");
    return;
  }
  if (IEditor* b = dynamic_cast<IEditor*>(getActive())) {
    b->saveAsButton();
    return;
  }
  // In theory, you could handle custom types here implementing a different API.
  rsl::ErrorDialog("Failed to save. Not saveable");
}

bool RootWindow::shouldClose() {
  if (!hasChildren()) {
    return true;
  }
  auto choice = unsavedProgressBox();

  if (choice == UnsavedProgressResult::CancelClose) {
    return false;
  }

  if (choice == UnsavedProgressResult::Save) {
    saveButton();
  }

  return true;
}

} // namespace riistudio::frontend
