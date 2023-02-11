#pragma once

#include <core/util/oishii.hpp>
#include <frontend/IEditor.hpp>
#include <frontend/editor/StudioWindow.hpp>
#include <frontend/level_editor/AutoHistory.hpp>
#include <frontend/widgets/PropertyEditorWidget.hpp>
#include <librii/egg/PBLM.hpp>
#include <librii/sp/DebugClient.hpp>
#include <rsl/FsDialog.hpp>

namespace riistudio::frontend {

// Implements a single tab for now
class BblmEditorTabSheet : private PropertyEditorWidget {
public:
  BblmEditorTabSheet(std::function<void(void)> draw) : m_drawTab(draw) {}
  void Draw() {
    DrawTabWidget(false);
    Tabs();
  }

private:
  std::vector<std::string> TabTitles() override { return {"BBLM"}; }
  bool Tab(int index) override {
    m_drawTab();
    return true;
  }

  std::function<void(void)> m_drawTab;
};

class BblmEditorPropertyGrid {
public:
  void Draw();

  librii::egg::BLM m_blm;
};

class BblmEditor : public frontend::StudioWindow, public IEditor {
public:
  BblmEditor()
      : StudioWindow("BBLM Editor: <unknown>", false),
        m_sheet([pEditor = this]() { pEditor->m_grid.Draw(); }) {
    setWindowFlag(ImGuiWindowFlags_MenuBar);
  }
  BblmEditor(const BblmEditor&) = delete;
  BblmEditor(BblmEditor&&) = delete;

  void draw_() override {
    m_sheet.Draw();
    m_history.update(m_grid.m_blm);
  }
  ImGuiID buildDock(ImGuiID root_id) override {
    // ImGui::DockBuilderDockWindow("Properties", root_id);
    return root_id;
  }

  void openFile(std::span<const u8> buf, std::string_view path) override {
    oishii::DataProvider view(buf | rsl::ToList(), std::string(path));
    oishii::BinaryReader reader(view.slice());
    rsl::SafeReader safe(reader);
    auto bblm = librii::egg::PBLM_Read(safe);
    if (!bblm) {
      rsl::error("Failed to read BBLM");
      return;
    }
    auto blm = librii::egg::From_PBLM(*bblm);
    if (!blm) {
      rsl::error("Failed to parse BBLM");
    }
    m_grid.m_blm = *blm;
    m_path = path;
  }
  void saveAs(std::string_view path) {
    auto writer = write();
    OishiiFlushWriter(writer, path);
  }
  std::string discordStatus() const override {
    return "Editing a bloom file (.bblm)";
  }

  oishii::Writer write() const {
    oishii::Writer writer(0x50);
    auto bblm = librii::egg::To_PBLM(m_grid.m_blm);
    librii::egg::PBLM_Write(writer, bblm);
    return writer;
  }

  std::string getFilePath() const { return m_path; }

private:
  BblmEditorPropertyGrid m_grid;
  BblmEditorTabSheet m_sheet;
  std::string m_path;
  lvl::AutoHistory<librii::egg::BLM> m_history;
};

} // namespace riistudio::frontend
