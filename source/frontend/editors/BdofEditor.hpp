#pragma once

#include <core/util/oishii.hpp>
#include <frontend/IEditor.hpp>
#include <frontend/legacy_editor/StudioWindow.hpp>
#include <frontend/widgets/AutoHistory.hpp>
#include <frontend/widgets/PropertyEditorWidget.hpp>
#include <librii/egg/BDOF.hpp>
#include <librii/sp/DebugClient.hpp>
#include <rsl/FsDialog.hpp>

namespace riistudio::frontend {

// Implements a single tab for now
class BdofEditorTabSheet {
public:
  void Draw(std::function<void(void)> draw) {
    std::vector<std::string> titles{
        "BDOF",
    };
    std::function<bool(int)> drawTab = [&](int index) {
      if (index != 0) {
        return false;
      }
      draw();
      return true;
    };
    DrawPropertyEditorWidgetV2(m_state, drawTab, titles);
  }
  PropertyEditorState m_state;
};

class BdofEditorPropertyGrid {
public:
  void Draw();

  librii::egg::DOF m_dof;
};

class BdofEditor : public frontend::StudioWindow, public IEditor {
public:
  BdofEditor() : StudioWindow("BDOF Editor: <unknown>", DockSetting::None) {
    // setWindowFlag(ImGuiWindowFlags_MenuBar);
  }
  BdofEditor(const BdofEditor&) = delete;
  BdofEditor(BdofEditor&&) = delete;

  void draw_() override {
    setName("BDOF Editor: " + m_path);
    if (ImGui::Button("DEBUG connect")) {
      debugConnect();
    }
    {
      util::ConditionalActive g(m_state == State::Connected);
      ImGui::SameLine();
      if (ImGui::Button("DEBUG get")) {
        debugGet();
      }
      ImGui::SameLine();
      if (ImGui::Button("DEBUG send")) {
        debugSend();
      }
      ImGui::Checkbox("Automatically sync with game", &m_sync);
    }
    switch (m_state) {
    case State::None:
      ImGui::Text("Not connected");
      break;
    case State::Connecting:
      ImGui::Text("Waiting for game to initiate connection...");
      break;
    case State::Connected:
      ImGui::Text("Connected.");
      break;
    }

    auto last = m_grid.m_dof;
    m_sheet.Draw([&]() { m_grid.Draw(); });
    if (m_sync && last != m_grid.m_dof) {
      debugSend();
    }

    m_history.update(m_grid.m_dof);
  }
  ImGuiID buildDock(ImGuiID root_id) override {
    // ImGui::DockBuilderDockWindow("Properties", root_id);
    return root_id;
  }

  std::string discordStatus() const override {
    return "Editing a depth-of-field (.bdof) file.";
  }

  void openFile(std::span<const u8> buf, std::string_view path) {
    auto dof = librii::egg::ReadDof(buf, path);
    if (!dof) {
      rsl::error("Failed to parse BDOF");
    }
    m_grid.m_dof = *dof;
    m_path = path;
  }
  void saveAs(std::string_view path) { WriteDof(m_grid.m_dof, path); }

  void saveButton() override {
    rsl::trace("Attempting to save to {}", m_path);
    if (m_path.empty()) {
      saveAsButton();
      return;
    }
    saveAs(m_path);
  }
  void saveAsButton() override {
    auto default_filename = std::filesystem::path(m_path).filename();
    std::vector<std::string> filters{"EGG Binary BDOF (*.bdof)", "*.bdof",
                                     "EGG Binary PDOF (*.pdof)", "*.pdof"};
    auto results =
        rsl::SaveOneFile("Save File"_j, default_filename.string(), filters);
    if (!results) {
      rsl::ErrorDialog("No saving - No file selected");
      return;
    }
    auto path = results->string();

    // Just autofill BDOF for now
    if (!path.ends_with(".bdof") && !path.ends_with(".pdof")) {
      path += ".bdof";
    }

    saveAs(path);
  }

private:
  BdofEditorPropertyGrid m_grid;
  BdofEditorTabSheet m_sheet;
  std::string m_path;
  enum class State {
    None,
    Connecting,
    Connected,
  };
  State m_state = State::None;
  bool m_sync = false;
  lvl::AutoHistory<librii::egg::DOF> m_history;

  void debugGet() {
    librii::sp::DebugClient::GetSettingsAsync(
        60, [pE = this](std::expected<librii::sp::DebugClient::Settings,
                                      librii::sp::DebugClient::Error>
                            settings) {
          if (!settings) {
            rsl::ErrorDialog("Failed to read debug settings");
            return;
          }
          pE->openFile(settings->bdof, pE->m_path);
        });
  }
  void debugConnect() {
    if (m_state != State::None) {
      rsl::ErrorDialog("Can only call debugConnect() from State::None");
      return;
    }
    rsl::ErrorDialog("WARNING: Debug connection uses the prototype protocol. "
                     "Now waiting for the game to initiate a conection.");
    m_state = State::Connecting;
    librii::sp::DebugClient::ConnectAsync(
        "127.0.0.1:1234", 60,
        [pE = this](std::optional<librii::sp::DebugClient::Error> err) {
          if (err.has_value()) {
            rsl::ErrorDialogFmt("Failed to conect: {}",
                                magic_enum::enum_name(*err));
            pE->m_state = State::None;
            return;
          }
          pE->m_state = State::Connected;
        });
  }
  void debugSend() {
    auto writer = librii::egg::WriteDofMemory(m_grid.m_dof);
    auto buf = writer.takeBuf();
    if (buf.size() == 0x50) {
      std::array<u8, 0x50> d;
      for (u8 i = 0; i < 0x50; ++i) {
        d[i] = buf[i];
      }
      librii::sp::DebugClient::SendSettingsAsync(
          librii::sp::DebugClient::Settings{.bdof = d});
    }
  }
};

} // namespace riistudio::frontend
