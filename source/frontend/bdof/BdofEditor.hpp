#pragma once

#include <core/util/oishii.hpp>
#include <frontend/editor/StudioWindow.hpp>
#include <frontend/widgets/PropertyEditorWidget.hpp>
#include <librii/egg/BDOF.hpp>

namespace riistudio::frontend {

// Implements a single tab for now
class BdofEditorTabSheet : private PropertyEditorWidget {
public:
  BdofEditorTabSheet(std::function<void(void)> draw) : m_drawTab(draw) {}
  void Draw() { DrawTabWidget(false); }

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

  void draw_() override { m_grid.Draw(); }
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
    oishii::Writer writer(0x50);
    auto bdof = librii::egg::To_BDOF(m_grid.m_dof);
    librii::egg::bin::BDOF_Write(writer, bdof);
    OishiiFlushWriter(writer, path);
  }

  std::string getFilePath() const { return m_path; }

private:
  BdofEditorPropertyGrid m_grid;
  BdofEditorTabSheet m_sheet;
  std::string m_path;
};

} // namespace riistudio::frontend
