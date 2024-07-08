#pragma once

#include <fa5/IconsFontAwesome5.h>
#include <string.h>

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

bool loadFonts(float fontSize = 16.0f);

} // namespace riistudio::frontend
