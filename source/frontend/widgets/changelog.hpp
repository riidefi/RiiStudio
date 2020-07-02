#include <core/util/gui.hpp>   // ImGui::Text
#include <frontend/applet.hpp> // core::Markdown

namespace riistudio {

void DrawChangeLog(bool* show = nullptr) {
  if (show != nullptr && !*show)
    return;

  if (ImGui::Begin("Changelog", show, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::SetWindowFontScale(1.3f);
    ImGui::Text("RiiStudio: Alpha 2.1 Pre-Release");
    const std::string& markdownText = R"(
UI:
  * Support for adding samplers
  * Fixed texture SRT issues
  * Icons
  * Proper texture and sampler selectors
Misc
  * Bugfixes

Alpha 2.0
===
UI Overhaul:
  * New TEV, Lighting, Pixel Shader, Swap Table, Draw Call, Vertex Data, and Texture property editors.

BMD Saving:
  * Note: BDL files currently will be saved as BMD. This incurs no data loss.

Misc:
  * Added vertical tabs and header visibility toggles to the property editor.
  * Fixed several shadergen bugs.
  * Added support for BRRES files with basic rigging.
  * Multiple instances of property editors on the same tab can work in parallel.
  * Fixed bug where rotation was converted to radians twice for BMD SRT matrices.
  * Fixed culling mode being read incorrectly in BMD files.
  * Automatically set camera clipping planes.
  * Fixed handling compression LUTs in BMD files.
  * Fixed rendering BMD files with varying vertex descriptors.
)";
    frontend::Markdown(markdownText);

    ImGui::End();
  }
}

} // namespace riistudio
