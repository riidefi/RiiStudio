#include <core/applet.hpp> // core::Markdown
#include <core/util/gui.hpp>

#include <frontend/ThemeManager.hpp>

namespace riistudio {

void DrawThemeEditor(ThemeManager::BasicTheme& theme,
                   float& fontGlobalScale, bool* show) {
  if (show != nullptr && !*show)
    return;

  if (ImGui::Begin("Theme Editor", show, ImGuiWindowFlags_AlwaysAutoResize)) {
    int sel = static_cast<int>(theme);
    ImGui::Combo("Theme", &sel, ThemeManager::ThemeNames);
    theme = static_cast<ThemeManager::BasicTheme>(sel);
    ImGui::SliderFloat("Font Scale", &fontGlobalScale, 0.1f, 2.0f);
    ImGui::End();
  }
}

} // namespace riistudio
