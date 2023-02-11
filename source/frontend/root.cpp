#include "root.hpp"
#include "LeakDebug.hpp"
#include <core/3d/gl.hpp>
#include <core/util/timestamp.hpp>
#include <frontend/Localization.hpp>
#include <frontend/editor/EditorWindow.hpp>
#include <frontend/widgets/fps.hpp>
#include <imcxx/Widgets.hpp>
#include <imgui_markdown.h>
#include <oishii/reader/binary_reader.hxx>
#include <oishii/writer/binary_writer.hxx>
#include <pfd/portable-file-dialogs.h>
#include <plugins/api.hpp>

// Experimental conversion
#include <plugins/g3d/collection.hpp>
#include <plugins/j3d/Scene.hpp>

#include <vendor/stb_image.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <frontend/bdof/BblmEditor.hpp>
#include <frontend/bdof/BdofEditor.hpp>
#include <frontend/level_editor/LevelEditor.hpp>

#include <frontend/Fonts.hpp>
#include <librii/szs/SZS.hpp>
#include <rsl/FsDialog.hpp>

#include <rsl/Discord.hpp>

IMPORT_STD;

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
  DoLeakCheck();
  fileHostProcess();

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

    EditorWindow* ed =
        getActive() ? dynamic_cast<EditorWindow*>(getActive()) : nullptr;

    drawMenuBar(ed);

    MSVCWarningWindow();

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

  const auto wflags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove |
                      ImGuiWindowFlags_AlwaysAutoResize;

  ImGui::OpenPopup("Importer"_j);
  {
    auto pos = ImGui::GetWindowPos();
    pos.x += (ImGui::GetWindowWidth() - 800.0f) / 2;
    pos.y += (ImGui::GetWindowHeight() - 600.0f) / 2;
    ImGui::SetNextWindowPos(pos);
    ImGui::SetNextWindowSizeConstraints({800.0f, 0.0f}, {800.0f, 600.0f});
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
        rsl::trace("Attempting to save to {}", ed->getFilePath());
        if (!ed->getFilePath().empty()) {
          save(ed->getFilePath());
        } else {
          saveAs();
        }
      } else {
        // Try anyway (.bdof)
        saveAs();
      }
    }
#if !defined(__EMSCRIPTEN__)
    if (ImGui::MenuItem("Save As"_j)) {
      if (ed) {
        saveAs();
      } else {
        // Try anyway (.bdof)
        saveAs();
      }
    }
#endif
    ImGui::EndMenu();
  }
}
void RootWindow::onFileOpen(FileData data, OpenFilePolicy policy) {
  rsl::info("Opening file: {}", data.mPath.c_str());

  std::span<const u8> span(data.mData.get(), data.mData.get() + data.mLen);

  if (data.mPath.ends_with("szs")) {
    auto pWin = std::make_unique<lvl::LevelEditorWindow>();

    pWin->openFile(span, data.mPath);

    attachWindow(std::move(pWin));
    return;
  }
  if (data.mPath.ends_with("bdof")) {
    auto pWin = std::make_unique<BdofEditor>();

    pWin->openFile(span, data.mPath);

    attachWindow(std::move(pWin));
    return;
  }
  if (data.mPath.ends_with("bblm")) {
    auto pWin = std::make_unique<BblmEditor>();

    pWin->openFile(span, data.mPath);

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

  mDiscordRpc.connect();

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
  if (getActive() != nullptr) {
    if (auto* b = dynamic_cast<IEditor*>(getActive())) {
      rsl::trace("Saving to {}", path);
      b->saveAs(path);
      return;
    }
  }

  rsl::ErrorDialog("Failed to save. Not saveable");
}
void RootWindow::saveAs() {
  std::vector<std::string> filters;

  if (BdofEditor* b = dynamic_cast<BdofEditor*>(getActive())) {
    auto default_filename = std::filesystem::path(b->getFilePath()).filename();
    filters.push_back("EGG Binary BDOF (*.bdof)");
    filters.push_back("*.bdof");
    filters.push_back("EGG Binary PDOF (*.pdof)");
    filters.push_back("*.pdof");
    auto results =
        rsl::SaveOneFile("Save File"_j, default_filename.string(), filters);
    if (!results) {
      rsl::ErrorDialog("No saving - No file selected");
      return;
    }
    auto path = results->string();

    // Just autofill BDOF for now
    if (!path.ends_with(".bdof") && !path.ends_with(".pdof")) {
      path += ".bdof";
    }

    save(path);
    return;
  }
  if (BblmEditor* b = dynamic_cast<BblmEditor*>(getActive())) {
    auto default_filename = std::filesystem::path(b->getFilePath()).filename();
    filters.push_back("EGG Binary BBLM (*.bblm)");
    filters.push_back("*.bblm");
    filters.push_back("EGG Binary PBLM (*.pblm)");
    filters.push_back("*.pblm");
    auto results =
        rsl::SaveOneFile("Save File"_j, default_filename.string(), filters);
    if (!results) {
      rsl::ErrorDialog("No saving - No file selected");
      return;
    }
    auto path = results->string();

    // Just autofill BBLM for now
    if (!path.ends_with(".bblm") && !path.ends_with(".pblm")) {
      path += ".bblm";
    }

    save(path);
    return;
  }

  EditorWindow* ed =
      getActive() ? dynamic_cast<EditorWindow*>(getActive()) : nullptr;
  if (ed == nullptr)
    return;

  const kpi::INode* node = &ed->getDocument().getRoot();

  auto default_filename = std::filesystem::path(ed->getFilePath()).filename();
  if (dynamic_cast<const riistudio::j3d::Collection*>(node) != nullptr) {
    filters.push_back("Binary Model Data (*.bmd)"_j);
    filters.push_back("*.bmd");
    default_filename.replace_extension(".bmd");
  } else if (dynamic_cast<const riistudio::g3d::Collection*>(node) != nullptr) {
    filters.push_back("Binary Resource (*.brres)"_j);
    filters.push_back("*.brres");
    default_filename.replace_extension(".brres");
  }

  filters.push_back("All Files");
  filters.push_back("*");

  auto results =
      rsl::SaveOneFile("Save File"_j, default_filename.string(), filters);
  if (!results)
    return;
  auto path = results->string();

  if (dynamic_cast<const riistudio::j3d::Collection*>(node) != nullptr) {
    if (!path.ends_with(".bmd"))
      path.append(".bmd");
  } else if (dynamic_cast<const riistudio::g3d::Collection*>(node) != nullptr) {
    if (!path.ends_with(".brres"))
      path.append(".brres");
  }

  save(path);
}

RootWindow::UnsavedProgressResult RootWindow::unsavedProgressBox() {
  auto box = pfd::message("Unsaved Files", //
                          "Do you want to save before closing?",
                          pfd::choice::yes_no_cancel, pfd::icon::warning);

  switch (box.result()) {
  case pfd::button::yes:
    return UnsavedProgressResult::Save;
  case pfd::button::no:
    return UnsavedProgressResult::DontSave;
  case pfd::button::cancel:
  default:
    return UnsavedProgressResult::CancelClose;
  }
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
    EditorWindow* ed =
        getActive() ? dynamic_cast<EditorWindow*>(getActive()) : nullptr;
    if (ed == nullptr) {
      rsl::ErrorDialog("Current editor failed to save.");
      return false;
    }
    rsl::info("Attempting to save to {}", ed->getFilePath());
    if (!ed->getFilePath().empty()) {
      save(ed->getFilePath());
    } else {
      saveAs();
    }
  }

  return true;
}

} // namespace riistudio::frontend
