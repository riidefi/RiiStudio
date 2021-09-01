#include "ImporterWindow.hpp"
#include <algorithm>
#include <imgui/imgui.h>
#include <vendor/fa5/IconsFontAwesome5.h>

namespace riistudio::frontend {

void ImporterWindow::draw() {
  assert(!mDone && "ImporterWindow was called after it completed.");
  if (mDone)
    return;

  ImVec4 error_col{200.0f / 255.0f, 12.0f / 255.0f, 12.0f / 255.0f, 1.0f};

  const bool finished = succeeded() || failed();

  if (failed()) {
    ImGui::TextColored(error_col, "%s", describeError());
  } else if (succeeded()) {
    ImGui::TextUnformatted("Success"_j);
  } else {
    // Progress states
    switch (result) {
    case State::ConfigureProperties:
      ImGui::TextUnformatted("Configure Properties"_j);
      importerRender();
      break;
    case State::AmbiguousConstructible: {
      ImGui::TextUnformatted("Pick which file format to create."_j);
      bool answered = false;
      for (auto& choice : choices) {
        const char* format = choice.c_str();
        if (choice.find("g3d") != std::string::npos)
          format = "BRRES Model"_j;
        else if (choice.find("j3d") != std::string::npos)
          format = "BMD Model"_j;
        if (ImGui::Button(format,
                          ImVec2{ImGui::GetContentRegionAvailWidth(), 0.0f})) {
          data_id = choice;
          result = State::Constructible;
          answered = true;
        }
      }
      // Keep asking..
      if (!answered)
        return;

      // The consumer will construct the state
      process();
      break;
    }
    case State::ResolveDependencies: {
      assert(transaction.has_value());
      unsigned num = 0;
      for (auto& vec : transaction->resolvedFiles)
        if (vec.empty())
          ++num;

      if (num == 0) {
        process();
        return;
      }

      ImGui::SetWindowFontScale(2.0f);
      ImGui::TextColored(
          error_col, "%s",
          "Missing textures! Please drop the following textures into "
          "this window."_j);

      ImGui::Text("(%u Remaining)"_j, num);
      ImGui::SetWindowFontScale(1.0f);
      if (ImGui::BeginTable("Missing Textures"_j, 1)) {
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

  if (ImGui::Button(failed() ? "Quit"_j : "Next"_j,
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

static inline std::string getFileShortStripped(const std::string& path) {
  auto tmp = path.substr(path.rfind("\\") + 1);
  tmp = tmp.substr(0, tmp.rfind("."));
  return tmp;
}

void ImporterWindow::drop(FileData&& data) {
  // Assumption: individual files across folders are unique.
  // We may need to change this?

  const auto file = getFileShortStripped(data.mPath);

  for (int i = 0; i < transaction->unresolvedFiles.size(); ++i) {
    if (getFileShortStripped(transaction->unresolvedFiles[i]) == file) {
      auto& resolved = transaction->resolvedFiles[i];
      // Unfortunate..
      resolved.resize(data.mLen);
      std::memcpy(resolved.data(), data.mData.get(), data.mLen);
    }
  }
}

} // namespace riistudio::frontend
