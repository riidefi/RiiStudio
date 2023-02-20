#pragma once

// TODO: Don't depend on kpi::IOMessageClass
#include <LibBadUIFramework/Plugins.hpp>
#include <imgui/imgui.h>
#include <vendor/fa5/IconsFontAwesome5.h>

namespace riistudio::frontend {

struct Message {
  kpi::IOMessageClass message_class;
  std::string domain;
  std::string message_body;

  Message(kpi::IOMessageClass mclass, std::string&& mdomain, std::string&& body)
      : message_class(mclass), domain(std::move(mdomain)),
        message_body(std::move(body)) {}
};
struct ErrorDialogList {
  void Draw(std::span<const Message> messages) {
    const auto entry_flags =
        ImGuiTableFlags_Borders | ImGuiTableFlags_Sortable |
        ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable;

    if (ImGui::BeginTable("Body", 3, entry_flags)) {
      ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthStretch, .1f);
      ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_WidthStretch, .3f);
      ImGui::TableSetupColumn("Body", ImGuiTableColumnFlags_WidthStretch, .6f);
      ImGui::TableHeadersRow();
      for (auto& msg : messages) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted([](kpi::IOMessageClass mclass) -> const char* {
          switch (mclass) {
          case kpi::IOMessageClass::None:
            return "Invalid";
          case kpi::IOMessageClass::Information:
            return (const char*)ICON_FA_BELL u8" Info";
          case kpi::IOMessageClass::Warning:
            return (const char*)ICON_FA_EXCLAMATION_TRIANGLE u8" Warning";
          case kpi::IOMessageClass::Error:
            return (const char*)ICON_FA_TIMES u8" Error";
          }
        }(msg.message_class));
        ImGui::TableSetColumnIndex(1);
        ImGui::TextUnformatted(msg.domain.c_str());
        ImGui::TableSetColumnIndex(2);
        ImGui::TextWrapped("%s", msg.message_body.c_str());
      }

      ImGui::EndTable();
    }
  }
};

} // namespace riistudio::frontend
