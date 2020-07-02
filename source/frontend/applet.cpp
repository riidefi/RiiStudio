#include "applet.hpp"
#include <fa5/IconsFontAwesome5.h>
#include <imgui/imgui.h>
#include <imgui_markdown.h>

namespace riistudio::frontend {

// You can make your own Markdown function with your prefered string container
// and markdown config.
static ImGui::MarkdownConfig mdConfig{
    nullptr,
    nullptr,
    reinterpret_cast<const char*>(ICON_FA_LINK),
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

// #ifdef BUILD_DIST
#define FONT_DIR "./fonts/"
// #else
// #define FONT_DIR "../../fonts/"
// #endif

  const char* default_font = FONT_DIR "Roboto-Medium.ttf";
  // const char* bold_font = FONT_DIR "Roboto-Bold.ttf";
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

Applet::Applet(const std::string& name) : plate::Platform(1280, 720, name) {

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
  mpGlfwWindow = (GLFWwindow*)getPlatformWindow();
#endif
}

Applet::~Applet() {}

void Applet::rootDraw() {}
void Applet::rootCalc() {
  detachClosedChildren();
  draw(); // Call down to window manager
}

} // namespace riistudio::frontend
