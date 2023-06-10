#pragma once

#include <LibBadUIFramework/ActionMenu.hpp>
#include <frontend/widgets/TextEdit.hpp>

namespace riistudio::frontend {

class RenameNode : public kpi::ActionMenu<kpi::IObject, RenameNode> {
  bool m_rename = false;
  TextEdit m_textEdit;

public:
  bool _context(kpi::IObject& obj) {
    // Really silly way to determine if an object can be renamed
    auto name = obj.getName();
    obj.setName(obj.getName() + "_");
    bool can_rename = obj.getName() != name;
    obj.setName(name);

    if (can_rename && ImGui::MenuItem("Rename"_j)) {
      m_rename = true;
      m_textEdit.Reset(name);
    }

    return false;
  }

  kpi::ChangeType _modal(kpi::IObject& obj) {
    if (m_rename) {
      m_rename = false;
      ImGui::OpenPopup("Rename");
    }
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter());
    const auto wflags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove |
                        ImGuiWindowFlags_AlwaysAutoResize;
    bool open = true;
    if (ImGui::BeginPopupModal("Rename", &open, wflags)) {
      RSL_DEFER(ImGui::EndPopup());
      const auto name = obj.getName();
      bool ok = m_textEdit.Modal(name);
      if (ImGui::Button("Rename") || ok) {
        const auto new_name = m_textEdit.String();
        if (name == new_name) {
          ImGui::CloseCurrentPopup();
          return kpi::NO_CHANGE;
        }
        obj.setName(new_name);
        ImGui::CloseCurrentPopup();
        return kpi::CHANGE;
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel") || !open) {
        ImGui::CloseCurrentPopup();
        return kpi::NO_CHANGE;
      }
    }

    return kpi::NO_CHANGE;
  }
};

} // namespace riistudio::frontend
