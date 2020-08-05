#include "ImporterWindow.hpp"
#include <algorithm>
#include <imgui/imgui.h>
#include <vendor/fa5/IconsFontAwesome5.h>

namespace riistudio::frontend {

static std::string getFileShort(const std::string& path) {
  return path.substr(path.rfind("\\") + 1);
}

void ImporterWindow::draw() {
  assert(!mDone && "ImporterWindow was called after it completed.");
  if (mDone)
    return;

  const bool finished = succeeded() || failed();

  if (failed()) {
    ImGui::TextUnformatted(describeError());
  } else if (succeeded()) {
    ImGui::Text("Success");
  } else {
    // Progress states
    switch (result) {
    case State::ConfigureProperties:
      ImGui::Text("Configure Properties");
      importerRender();
      break;
    case State::ResolveDependencies: {
      unsigned num = 0;
      for (auto& vec : transaction->resolvedFiles)
        if (vec.empty())
          ++num;

      if (num == 0) {
        process();
        return;
      }

      ImVec4 col{200.0f / 255.0f, 12.0f / 255.0f, 12.0f / 255.0f, 1.0f};
      ImGui::SetWindowFontScale(2.0f);
      ImGui::TextColored(
          col, "Missing textures! Please drop the following textures into "
               "this window.");

      ImGui::Text("(%u Remaining)", num);
      ImGui::SetWindowFontScale(1.0f);
      if (ImGui::BeginTable("Missing Textures", 1)) {
        for (int i = 0; i < transaction->unresolvedFiles.size(); ++i) {
          if (transaction->resolvedFiles[i].empty()) {
            ImGui::TableNextRow();
            ImGui::TextUnformatted(transaction->unresolvedFiles[i].c_str());
          }
        }
        ImGui::EndTable();
      }
      break;
    }
    case State::Communicating:
      ImGui::Text("Waiting on importer thread..");
      break;
    default:
      break;
    }
  }

  if (finished) {
    if (!mMessages.empty()) {
      drawMessages();
    } else {
      mDone = true;
      return;
    }
  }

  if (ImGui::Button("Next",
                    ImVec2{ImGui::GetContentRegionAvailWidth(), 0.0f})) {
    if (finished) {
      mDone = true;
      return;
    }

    process();
  }
}

void ImporterWindow::drawMessages() {
  // ImGui::SetWindowFontScale(1.2f);
  const auto entry_flags = ImGuiTableFlags_Borders | ImGuiTableFlags_Sortable |
                           ImGuiTableFlags_Resizable |
                           ImGuiTableFlags_Reorderable;

  if (ImGui::BeginTable("Body", 3, entry_flags)) {
    ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_None, .1f);
    ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_None, .3f);
    ImGui::TableSetupColumn("Body", ImGuiTableColumnFlags_None, .6f);
    ImGui::TableAutoHeaders();
    for (auto& msg : mMessages) {
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
  // ImGui::SetWindowFontScale(1.0f);
}

void ImporterWindow::drop(FileData&& data) {
  // Assumption: individual files across folders are unique.
  // We may need to change this?

  const auto file = getFileShort(data.mPath);

  for (int i = 0; i < transaction->unresolvedFiles.size(); ++i) {
    if (getFileShort(transaction->unresolvedFiles[i]) == file) {
      auto& resolved = transaction->resolvedFiles[i];
      // Unfortunate..
      resolved.resize(data.mLen);
      std::memcpy(resolved.data(), data.mData.get(), data.mLen);
    }
  }
}

} // namespace riistudio::frontend
