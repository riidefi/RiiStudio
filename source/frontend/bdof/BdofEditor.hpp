#pragma once

#include <core/util/oishii.hpp>
#include <frontend/editor/StudioWindow.hpp>
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

class BdofEditor : public frontend::StudioWindow {
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
    if (ImGui::Button("DEBUG get")) {
      debugGet();
    }
    if (ImGui::Button("DEBUG send")) {
      debugSend();
    }
    ImGui::Checkbox("Automatically sync with game", &m_sync);

    auto last = m_grid.m_dof;
    m_sheet.Draw();
    if (m_sync && last != m_grid.m_dof) {
      debugSend();
    }
  }
  ImGuiID buildDock(ImGuiID root_id) override {
    // ImGui::DockBuilderDockWindow("Properties", root_id);
    return root_id;
  }

  void openFile(std::span<const u8> buf, std::string path) {
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
  void saveAs(std::string path) {
    auto writer = write();
    OishiiFlushWriter(writer, path);
  }

  oishii::Writer write() {
    oishii::Writer writer(0x50);
    auto bdof = librii::egg::To_BDOF(m_grid.m_dof);
    librii::egg::bin::BDOF_Write(writer, bdof);
    return writer;
  }

  std::string getFilePath() const { return m_path; }

private:
  BdofEditorPropertyGrid m_grid;
  BdofEditorTabSheet m_sheet;
  std::string m_path;
  bool m_sync = false;

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
    librii::sp::DebugClient::ConnectAsync(
        "127.0.0.1:1234", 60,
        [](std::optional<librii::sp::DebugClient::Error> err) {});
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
