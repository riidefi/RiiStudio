#include "VersionInfo.hpp"

namespace riistudio::frontend {

void DrawSettingsWindow() {
  if (ImGui::Begin("Settings")) {
    DrawVersionInfo();
  }
  ImGui::End();
}

} // namespace riistudio::frontend
