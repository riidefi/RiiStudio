#include <frontend/ThemeManager.hpp> // ThemeManager
#include <imcxx/Widgets.hpp>         // ImGui::Begin

namespace riistudio {

struct ThemeData {
  ThemeManager::BasicTheme mTheme = ThemeManager::BasicTheme::Raikiri;
  float mFontGlobalScale = 1.0f;
  float mGlobalScale = 1.0f;
};

inline bool DrawThemeEditor(ThemeData& data, bool* show) {
  if (show != nullptr && !*show)
    return false;

  bool changed = false;

  if (show == nullptr ||
      ImGui::Begin("Theme Editor"_j, show, ImGuiWindowFlags_AlwaysAutoResize)) {
    {
      int sel = static_cast<int>(data.mTheme);
      ImGui::Combo("Theme"_j, &sel, ThemeManager::ThemeNames);
      changed |= (sel != static_cast<int>(data.mTheme));
      data.mTheme = static_cast<ThemeManager::BasicTheme>(sel);
    }
    changed |=
        ImGui::InputFloat("Font Scale"_j, &data.mFontGlobalScale, 0.1f, 2.0f);
    changed |=
        ImGui::InputFloat("Global Scale"_j, &data.mGlobalScale, 0.1f, 2.0f);

    if (show != nullptr)
      ImGui::End();
  }

  return changed;
}

} // namespace riistudio
