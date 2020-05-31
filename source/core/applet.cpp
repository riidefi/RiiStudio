#include "applet.hpp"
#include <fa5/IconsFontAwesome5.h>
#include <imgui/imgui.h>
#include <imgui/impl/imgui_impl_glfw.h>
#include <imgui/impl/imgui_impl_opengl3.h>

#include <vendor/imgui/impl/imgui_impl_sdl.h>
#include <vendor/imgui_markdown.h>

namespace riistudio::core {

// You can make your own Markdown function with your prefered string container
// and markdown config.
static ImGui::MarkdownConfig mdConfig{
    nullptr,
    nullptr,
    ICON_FA_LINK,
    {{NULL, true}, {NULL, true}, {NULL, false}}};

void Markdown(const std::string& markdown_) {
  // fonts for, respectively, headings H1, H2, H3 and beyond
  ImGui::Markdown(markdown_.c_str(), markdown_.length(), mdConfig);
}
static bool loadFonts(float fontSize = 12.0f) {
  ImGuiIO& io = ImGui::GetIO();
  ImFontConfig fontcfg;
  fontcfg.OversampleH = 8;
  fontcfg.OversampleV = 8;

#ifdef BUILD_DIST
#define FONT_DIR "./fonts/"
#else
#define FONT_DIR "../../fonts/"
#endif

  const char* default_font = FONT_DIR "Roboto-Medium.ttf";
  const char* bold_font = FONT_DIR "Roboto-Bold.ttf";
  const char* icon_font = FONT_DIR FONT_ICON_FILE_NAME_FAS;
#undef FONT_DIR

  io.Fonts->AddFontFromFileTTF(default_font, fontSize, &fontcfg);

  // mdConfig.headingFormats[0].font = io.Fonts->AddFontFromFileTTF(bold_font,
  // fontSize * 2 * 1.1f, &fontcfg); mdConfig.headingFormats[1].font =
  // io.Fonts->AddFontFromFileTTF(bold_font, fontSize * 2, &fontcfg);
  // mdConfig.headingFormats[2].font = mdConfig.headingFormats[1].font;

  static const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
  ImFontConfig icons_config;
  icons_config.MergeMode = true;
  icons_config.PixelSnapH = true;
  io.Fonts->AddFontFromFileTTF(icon_font, fontSize, &icons_config,
                               icons_ranges);

  return true;
}

Applet::Applet(const std::string& name) : GLWindow(1280, 720, name) {

  ImGuiStyle& style = ImGui::GetStyle();
  ImGuiIO& io = ImGui::GetIO();
  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    style.WindowRounding = 0.0f;
    style.Colors[ImGuiCol_WindowBg].w = 1.0f;
  }

  if (!loadFonts()) {
    fprintf(stderr, "Failed to load fonts");
  }
#ifdef RII_BACKEND_GLFW
  mpGlfwWindow = getGlfwWindow();
#endif
}

Applet::~Applet() {}

void Applet::frameRender() {
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
#ifdef DEBUG
static bool demo = true;
#endif
void Applet::frameProcess() {
  detachClosedChildren();

  ImGui_ImplOpenGL3_NewFrame();
  newFrame(); // TODO: Include GL3 in this?
  ImGui::NewFrame();

#ifdef DEBUG
  if (demo)
    ImGui::ShowDemoWindow(&demo);
#endif
  draw(); // Call down to window manager

  ImGui::Render();
}

} // namespace riistudio::core
