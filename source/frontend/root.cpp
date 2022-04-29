#include "root.hpp"
#include "LeakDebug.hpp"
#include <core/3d/gl.hpp>
#include <core/api.hpp>
#include <core/util/gui.hpp>
#include <core/util/timestamp.hpp>
#include <frontend/Localization.hpp>
#include <frontend/editor/EditorWindow.hpp>
#include <frontend/widgets/fps.hpp>
#include <fstream>
#include <imcxx/Widgets.hpp>
#include <imgui_markdown.h>
#include <oishii/reader/binary_reader.hxx>
#include <oishii/writer/binary_writer.hxx>
#include <pfd/portable-file-dialogs.h>

// Experimental conversion
#include <plugins/g3d/collection.hpp>
#include <plugins/j3d/Scene.hpp>

#include <vendor/stb_image.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <frontend/level_editor/LevelEditor.hpp>

#include <frontend/Fonts.hpp>
#include <librii/szs/SZS.hpp>

namespace llvm {
int DisableABIBreakingChecks;
} // namespace llvm

bool gIsAdvancedMode = false;

bool IsAdvancedMode() { return gIsAdvancedMode; }

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
  GLFWwindow* window = reinterpret_cast<GLFWwindow*>(platform_window);
  GLFWimage image;
  image.pixels = stbi_load(path, &image.width, &image.height, 0, 4);
  glfwSetWindowIcon(window, 1, &image);
  stbi_image_free(image.pixels);
#endif
}

void RootWindow::draw() {
  DoLeakCheck();
  fileHostProcess();

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

    EditorWindow* ed =
        getActive() ? dynamic_cast<EditorWindow*>(getActive()) : nullptr;

    drawMenuBar(ed);

    if (bDemo)
      ImGui::ShowDemoWindow(&bDemo);

    if (mCheckUpdate && mUpdater.IsOnline())
      mUpdater.draw();

    processImportersQueue();
    drawChildren();
  }
  imcxx::EndFullscreenWindow();
}
void RootWindow::drawStatusBar() {
  ImGui::SetWindowFontScale(1.1f);
  if (!hasChildren()) {
    ImGui::TextUnformatted("Drop a file to edit."_j);
  }
  ImGui::SetWindowFontScale(1.0f);
}
void RootWindow::processImportersQueue() {
  if (mImportersQueue.empty())
    return;

  auto& window = mImportersQueue.front();

  if (window.attachEditor()) {
    attachEditorWindow(
        std::make_unique<EditorWindow>(window.takeResult(), window.getPath()));
    mImportersQueue.pop();
    return;
  }

  if (window.abort()) {
    mImportersQueue.pop();
    return;
  }

  const auto wflags = ImGuiWindowFlags_NoCollapse;

  ImGui::OpenPopup("Importer"_j);
  {
    ImGui::SetNextWindowSize({800.0f, 00.0f});
    ImGui::BeginPopupModal("Importer"_j, nullptr, wflags);
    window.draw();
  }
  ImGui::EndPopup();
}
void RootWindow::drawMenuBar(riistudio::frontend::EditorWindow* ed) {
  // [File] [Settings] [Lang]   (---> WindowWidth-60) [FPS]
  if (ImGui::BeginMenuBar()) {
    drawFileMenu(ed);

    drawSettingsMenu();

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
void RootWindow::drawFileMenu(riistudio::frontend::EditorWindow* ed) {
  if (ImGui::BeginMenu("File"_j)) {
#if !defined(__EMSCRIPTEN__)
    if (ImGui::MenuItem("Open"_j)) {
      openFile();
    }
#endif
    if (ImGui::MenuItem("Save"_j)) {
      if (ed) {
        DebugReport("Attempting to save to %s\n", ed->getFilePath().c_str());
        if (!ed->getFilePath().empty())
          save(ed->getFilePath());
        else
          saveAs();
      } else {
        DebugReport("%s", "Cannot save.. nothing has been opened.\n"_j);
      }
    }
#if !defined(__EMSCRIPTEN__)
    if (ImGui::MenuItem("Save As"_j)) {
      if (ed)
        saveAs();
      else
        DebugReport("%s", "Cannot save.. nothing has been opened.\n"_j);
    }
#endif
    ImGui::EndMenu();
  }
}
void RootWindow::onFileOpen(FileData data, OpenFilePolicy policy) {
  DebugReport("Opening file: %s\n", data.mPath.c_str());

  if (data.mPath.ends_with("szs")) {
    auto pWin = std::make_unique<lvl::LevelEditorWindow>();

    pWin->openFile(
        std::span<const u8>(data.mData.get(), data.mData.get() + data.mLen),
        data.mPath);

    attachWindow(std::move(pWin));
    return;
  }

  if (mWantFile) {
    mReqData = std::move(data);
    mGotFile = true;
    return;
  }

  if (!mImportersQueue.empty()) {
    auto& top = mImportersQueue.front();

    if (top.acceptDrop()) {
      top.drop(std::move(data));
      return;
    }
  }

  mImportersQueue.emplace(std::move(data));
}
void RootWindow::attachEditorWindow(std::unique_ptr<EditorWindow> editor) {
  attachWindow(std::move(editor));
}

static std::optional<std::vector<uint8_t>> LoadLuigiCircuitSample() {
  auto szs = ReadFileData("./samp/luigi_circuit_brres.szs");
  if (!szs)
    return std::nullopt;

  rsl::byte_view szs_view{szs->mData.get(), szs->mLen};
  const auto expanded_size = librii::szs::getExpandedSize(szs_view);

  std::vector<uint8_t> brres(expanded_size);
  auto err = librii::szs::decode(brres, szs_view);
  if (err)
    return std::nullopt;

  return brres;
}

RootWindow::RootWindow()
    : Applet(std::string("RiiStudio "_j) + RII_TIME_STAMP) {
  spInstance = this;

  // Loads the plugins for file formats / importers
  InitAPI();

  // Without this, clicking in the viewport with a mouse would move the window
  // when undocked.
  ImGui::GetIO().ConfigWindowsMoveFromTitleBarOnly = true;

  SetWindowIcon(getPlatformWindow(), "icon.png");

  {
    auto brres = LoadLuigiCircuitSample();
    if (brres)
      dropDirect(std::move(*brres), "./samples/luigi_circuit.brres");
  }

  #if 0
  // IOS has 3:1 DPI
  mThemeData.mGlobalScale = .534f;
  mThemeData.mFontGlobalScale = .534f;
  #endif
}
RootWindow::~RootWindow() { DeinitAPI(); }

void RootWindow::save(const std::string& path) {
  EditorWindow* ed =
      getActive() ? dynamic_cast<EditorWindow*>(getActive()) : nullptr;
  if (ed != nullptr)
    ed->saveAs(path);
}
void RootWindow::saveAs() {
  std::vector<std::string> filters;

  EditorWindow* ed =
      getActive() ? dynamic_cast<EditorWindow*>(getActive()) : nullptr;
  if (ed == nullptr)
    return;

  const kpi::INode* node = &ed->getDocument().getRoot();

  if (dynamic_cast<const riistudio::j3d::Collection*>(node) != nullptr) {
    filters.push_back("Binary Model Data (*.bmd)"_j);
    filters.push_back("*.bmd");
  } else if (dynamic_cast<const riistudio::g3d::Collection*>(node) != nullptr) {
    filters.push_back("Binary Resource (*.brres)"_j);
    filters.push_back("*.brres");
  }

  filters.push_back("All Files");
  filters.push_back("*");

  auto results = pfd::save_file("Save File"_j, "", filters).result();
  if (results.empty())
    return;

  if (dynamic_cast<const riistudio::j3d::Collection*>(node) != nullptr) {
    if (!results.ends_with(".bmd"))
      results.append(".bmd");
  } else if (dynamic_cast<const riistudio::g3d::Collection*>(node) != nullptr) {
    if (!results.ends_with(".brres"))
      results.append(".brres");
  }

  save(results);
}

} // namespace riistudio::frontend
