#include <frontend/widgets/ReflectedPropertyGrid.hpp>

#include "BblmEditor.hpp"

namespace riistudio::frontend {

void BblmEditorPropertyGrid::Draw() {
  if (ImGui::BeginChild("HelpBox", ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 1.5f), true, ImGuiWindowFlags_NoScrollbar)) {
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), (const char*)ICON_FA_BOOK_OPEN);
    ImGui::SameLine();
    ImGui::Text("Documentation: ");
    ImGui::SameLine();
    ImGui::TextLinkOpenURL("https://wiki.tockdom.com/wiki/BBLM_(File_Format)");
  }
  ImGui::EndChild();
  
  ReflectedPropertyGrid grid;
  grid.Draw(m_blm);
}

} // namespace riistudio::frontend
