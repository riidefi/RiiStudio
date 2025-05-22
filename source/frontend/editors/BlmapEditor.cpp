#include <frontend/widgets/ReflectedPropertyGrid.hpp>

#include "BlmapEditor.hpp"

namespace riistudio::frontend {

void BlmapEditorPropertyGrid::Draw(librii::egg::LightTexture& tex) {
  if (ImGui::BeginChild("HelpBox", ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 1.5f), true, ImGuiWindowFlags_NoScrollbar)) {
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), (const char*)ICON_FA_BOOK_OPEN);
    ImGui::SameLine();
    ImGui::Text("Documentation: ");
    ImGui::SameLine();
    ImGui::TextLinkOpenURL("https://wiki.tockdom.com/wiki/BLMAP_(File_Format)");
  }
  ImGui::EndChild();
  
  tex.baseLayer = imcxx::EnumCombo("Base Layer Type", tex.baseLayer);

  ImGui::InputText("Texture Name", tex.textureName.data(),
                   tex.textureName.size());
  if (ImGui::BeginTabBar("Draw Settings")) {
    for (size_t i = 0; i < tex.drawSettings.size(); ++i) {
      auto& s = tex.drawSettings[i];

      bool enabled = tex.activeDrawSettings & (1 << i);
      char buf[32];
      snprintf(buf, sizeof(buf), "%s%i", enabled ? "*" : "",
               static_cast<int>(i));

      if (ImGui::BeginTabItem(buf)) {
        ImGui::Checkbox("Enabled", &enabled);
        if (enabled) {
          tex.activeDrawSettings |= (1 << i);
        } else {
          tex.activeDrawSettings &= ~(1 << i);
        }

        riistudio::util::ConditionalActive g(enabled);

        ImGui::SliderFloat("Effect Scale", &s.normEffectScale, 0.0f, 1.0f);
        s.pattern = imcxx::EnumCombo("Pattern", s.pattern);
        ImGui::EndTabItem();
      }
    }
    ImGui::EndTabBar();
#if 0
    if (ImGui::BeginTabItem("+", nullptr, ImGuiTabItemFlags_Trailing)) {
      tex.drawSettings.emplace_back();
      ImGui::EndTabItem();
    }
#endif
  }
}

} // namespace riistudio::frontend
