#include <core/applet.hpp> // core::Markdown
#include <core/util/gui.hpp>

namespace riistudio {

void DrawChangeLog(bool* show = nullptr) {
  if (show != nullptr && !*show)
    return;

  if (ImGui::Begin("Changelog", show, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::SetWindowFontScale(1.3f);
    ImGui::Text("RiiStudio: 1.1 Pre-Release");
    const std::string& markdownText = u8R"(
Thanks for all the feedback so far! I'll continue to improve the tool accordingly.

Camera controls have been reworked.
  * No more stuttering, snapping and other unfriendliness.
  * Thanks, Yoshi2, for helping with this.
BRRES files now have experimental support.
  * Static meshes only.
  * Corrupted models (notably those authored by the tool BrawlBox) are likely to fail.
Misc:
  * BMD: Fixed bug forcing backface culling.
  * UI: Theme picker is now properly sized.
  * UI: TEV stage operands now presented nicely. (read-only for now, overhaul soon)
  * UI: Mip level slider hidden when no mipmaps exist for a texture.
  * Debug: Decluttered console output.
  * Various bugfixes and performance improvements.
)";
    core::Markdown(markdownText);

    ImGui::End();
  }
}

} // namespace riistudio
