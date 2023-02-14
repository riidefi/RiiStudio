#pragma once

#include <core/util/oishii.hpp>
#include <frontend/IEditor.hpp>
#include <frontend/editor/StudioWindow.hpp>
#include <frontend/widgets/AutoHistory.hpp>
#include <frontend/widgets/PropertyEditorWidget.hpp>
#include <librii/egg/BDOF.hpp>
#include <librii/sp/DebugClient.hpp>
#include <rsl/FsDialog.hpp>

namespace riistudio::frontend {

// Implements a single tab for now
class BdofEditorTabSheet : private PropertyEditorWidget {
public:
  BdofEditorTabSheet(std::function<void(void)> draw) : m_drawTab(draw) {}
  void Draw() {
    DrawTabWidget(false);
    Tabs();
  }

private:
  std::vector<std::string> TabTitles() override { return {"BDOF"}; }
  bool Tab(int index) override {
    m_drawTab();
    return true;
  }

  std::function<void(void)> m_drawTab;
};

class BdofEditorPropertyGrid {
public:
  void Draw();

  librii::egg::DOF m_dof;
};

class BdofEditor : public frontend::StudioWindow, public IEditor {
public:
  BdofEditor()
      : StudioWindow("BDOF Editor: <unknown>", false),
        m_sheet([pEditor = this]() { pEditor->m_grid.Draw(); }) {
    setWindowFlag(ImGuiWindowFlags_MenuBar);
  }
  BdofEditor(const BdofEditor&) = delete;
  BdofEditor(BdofEditor&&) = delete;

  void draw_() override {
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
    m_sheet.Draw();
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
    oishii::DataProvider view(buf | rsl::ToList(), path);
    oishii::BinaryReader reader(view.slice());
    rsl::SafeReader safe(reader);
    auto bdof = librii::egg::bin::BDOF_Read(safe);
    if (!bdof) {
      rsl::error("Failed to read BDOF");
      return;
    }
    auto dof = librii::egg::From_BDOF(*bdof);
    if (!dof) {
      rsl::error("Failed to parse BDOF");
    }
    m_grid.m_dof = *dof;
    m_path = path;
  }
  void saveAs(std::string_view path) {
    auto writer = write();
    OishiiFlushWriter(writer, path);
  }

  oishii::Writer write() const {
    oishii::Writer writer(0x50);
    auto bdof = librii::egg::To_BDOF(m_grid.m_dof);
    librii::egg::bin::BDOF_Write(writer, bdof);
    return writer;
  }

  std::string getFilePath() const { return m_path; }

  void saveButton() override {
    rsl::trace("Attempting to save to {}", getFilePath());
    if (getFilePath().empty()) {
      saveAsButton();
      return;
    }
    saveAs(getFilePath());
  }
  void saveAsButton() override {
    std::vector<std::string> filters;
    auto default_filename = std::filesystem::path(getFilePath()).filename();
    filters.push_back("EGG Binary BDOF (*.bdof)");
    filters.push_back("*.bdof");
    filters.push_back("EGG Binary PDOF (*.pdof)");
    filters.push_back("*.pdof");
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
    auto writer = write();
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
