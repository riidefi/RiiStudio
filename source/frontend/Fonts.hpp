#pragma once

#include <fa5/IconsFontAwesome5.h>
#include <imgui/imgui.h>

namespace riistudio::frontend {

static inline bool loadFonts(float fontSize = 16.0f) {
  ImGuiIO& io = ImGui::GetIO();
  ImFontConfig fontcfg;
  fontcfg.OversampleH = 8;
  fontcfg.OversampleV = 8;

// #ifdef BUILD_DIST
#define FONT_DIR "./fonts/"
  // #else
  // #define FONT_DIR "../../fonts/"
  // #endif

  // const char* default_font = FONT_DIR "Roboto-Medium.ttf";
  // const char* bold_font = FONT_DIR "Roboto-Bold.ttf";
  const char* icon_font = FONT_DIR FONT_ICON_FILE_NAME_FAS;

  const char* jpn_font = FONT_DIR "NotoSansCJKjp-Black.otf";
#undef FONT_DIR

  // io.Fonts->AddFontFromFileTTF(default_font, fontSize, &fontcfg);

  io.Fonts->AddFontFromFileTTF(jpn_font, fontSize, &fontcfg,
                               io.Fonts->GetGlyphRangesJapanese());

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

} // namespace riistudio::frontend
