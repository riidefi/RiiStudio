#include <core/util/gui.hpp>   // ImGui::Text
#include <frontend/applet.hpp> // core::Markdown

namespace riistudio {

void DrawChangeLog(bool* show, std::string markdownText) {
  if (show != nullptr && !*show)
    return;

  if (ImGui::Begin("Changelog", show, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::SetWindowFontScale(1.3f);
    frontend::Markdown(markdownText);
    ImGui::End();
  }
}

} // namespace riistudio
