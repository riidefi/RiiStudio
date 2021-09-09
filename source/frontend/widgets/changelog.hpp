#include <core/util/gui.hpp>   // ImGui::Text

namespace riistudio {

void DrawChangeLog(bool* show, std::string markdownText) {
  if (show != nullptr && !*show)
    return;

  if (ImGui::Begin("Changelog"_j, show, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::SetWindowFontScale(1.3f);
    util::Markdown(markdownText);
    ImGui::End();
  }
}

} // namespace riistudio
