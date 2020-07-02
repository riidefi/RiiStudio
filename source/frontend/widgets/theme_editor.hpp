#include <core/util/gui.hpp>         // ImGui::Begin
#include <frontend/ThemeManager.hpp> // ThemeManager

namespace riistudio {

bool DrawThemeEditor(ThemeManager::BasicTheme& theme, float& fontGlobalScale,
                     bool* show) {
  if (show != nullptr && !*show)
    return false;

  bool changed = false;

  if (show == nullptr ||
      ImGui::Begin("Theme Editor", show, ImGuiWindowFlags_AlwaysAutoResize)) {
    int sel = static_cast<int>(theme);
    ImGui::Combo("Theme", &sel, ThemeManager::ThemeNames);
    if (sel != static_cast<int>(theme))
      changed = true;
    theme = static_cast<ThemeManager::BasicTheme>(sel);
    ImGui::InputFloat("Font Scale", &fontGlobalScale, 0.1f, 2.0f);

    if (show != nullptr)
      ImGui::End();
  }

  return changed;
}

} // namespace riistudio
