#pragma once

#include <fa5/IconsFontAwesome5.h>
#include <imgui/imgui.h>

#include <rsl/Log.hpp>

namespace riistudio::frontend {

namespace Fonts {

#define FONT_DIR "./fonts/"

// FontAwesome5: 0.19 MB
constexpr const char* sFontAwesome5 = FONT_DIR FONT_ICON_FILE_NAME_FAS;

// NotoSans: 17 MB
constexpr const char* sJpnFont = FONT_DIR "NotoSansCJKjp-Black.otf";

// Roboto: 0.16 MB
constexpr const char* sEnUsFont = FONT_DIR "Roboto-Medium.ttf";

#undef FONT_DIR

// NotoSans is over 17MB large, and is already compressed.
//
#ifdef __EMSCRIPTEN__
constexpr const char* sTextFont = sEnUsFont;
#else
constexpr const char* sTextFont = sJpnFont;
#endif

inline bool IsJapaneseSupported() { return !strcmp(sTextFont, sJpnFont); }

} // namespace Fonts

static inline void TryAddFont(const char* path, float fontSize,
                              ImFontConfig* config = nullptr,
                              const ImWchar* glyph_ranges = nullptr) {
  rsl::info("[FONT] Loading {}", path);
  const void* result = ImGui::GetIO().Fonts->AddFontFromFileTTF(
      path, fontSize, config, glyph_ranges);
  if (result == nullptr) {
    rsl::info("[FONT] Failed to load {}", path);
  } else {
    rsl::info("[FONT] Loaded {}", path);
  }
}

static inline bool loadFonts(float fontSize = 16.0f) {
  ImGuiIO& io = ImGui::GetIO();

  {
    ImFontConfig fontcfg;
    fontcfg.OversampleH = 8;
    fontcfg.OversampleV = 8;

    TryAddFont(Fonts::sTextFont, fontSize, &fontcfg,
               io.Fonts->GetGlyphRangesJapanese());
  }

  // mdConfig.headingFormats[0].font = io.Fonts->AddFontFromFileTTF(bold_font,
  // fontSize * 2 * 1.1f, &fontcfg); mdConfig.headingFormats[1].font =
  // io.Fonts->AddFontFromFileTTF(bold_font, fontSize * 2, &fontcfg);
  // mdConfig.headingFormats[2].font = mdConfig.headingFormats[1].font;

  {
    static const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;

    TryAddFont(Fonts::sFontAwesome5, fontSize, &icons_config, icons_ranges);
  }

  return true;
}

} // namespace riistudio::frontend
