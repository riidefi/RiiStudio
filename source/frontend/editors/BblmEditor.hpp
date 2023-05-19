#pragma once

#include <core/util/oishii.hpp>
#include <frontend/IEditor.hpp>
#include <frontend/editor/StudioWindow.hpp>
#include <frontend/widgets/AutoHistory.hpp>
#include <frontend/widgets/PropertyEditorWidget.hpp>
#include <librii/egg/PBLM.hpp>
#include <librii/sp/DebugClient.hpp>
#include <rsl/FsDialog.hpp>

namespace riistudio::frontend {

// Implements a single tab for now
class BblmEditorTabSheet : private PropertyEditorWidget {
public:
  void Draw(std::function<void(void)> draw) {
    // No concurrent access
    assert(m_drawTab == nullptr);
    m_drawTab = draw;
    DrawTabWidget(false);
    Tabs();
    m_drawTab = nullptr;
  }

private:
  std::vector<std::string> TabTitles() override { return {"BBLM"}; }
  bool Tab(int index) override {
    m_drawTab();
    return true;
  }

  std::function<void(void)> m_drawTab = nullptr;
};

class BblmEditorPropertyGrid {
public:
  void Draw();

  librii::egg::BLM m_blm;
};

class BblmEditor : public frontend::StudioWindow, public IEditor {
public:
  BblmEditor() : StudioWindow("BBLM Editor: <unknown>", DockSetting::None) {
    // setWindowFlag(ImGuiWindowFlags_MenuBar);
  }
  BblmEditor(const BblmEditor&) = delete;
  BblmEditor(BblmEditor&&) = delete;

  void draw_() override {
    setName("BBLM Editor: " + m_path);
    m_sheet.Draw([&]() { m_grid.Draw(); });
    m_history.update(m_grid.m_blm);
  }
  ImGuiID buildDock(ImGuiID root_id) override {
    // ImGui::DockBuilderDockWindow("Properties", root_id);
    return root_id;
  }
  void openFile(std::span<const u8> buf, std::string_view path) {
    auto blm = librii::egg::ReadBLM(buf, path);
    if (!blm) {
      rsl::error(blm.error());
      rsl::ErrorDialog(blm.error());
      return;
    }
    m_grid.m_blm = *blm;
    m_path = path;
  }
  std::string discordStatus() const override {
    return "Editing a bloom file (.bblm)";
  }
  void saveButton() override {
    if (m_path.empty()) {
      saveAsButton();
      return;
    }
    WriteBLM(m_grid.m_blm, m_path);
  }
  void saveAsButton() override {
    auto default_filename = std::filesystem::path(m_path).filename();
    std::vector<std::string> filters{"EGG Binary BBLM (*.bblm)", "*.bblm",
                                     "EGG Binary PBLM (*.pblm)", "*.pblm"};
    auto results =
        rsl::SaveOneFile("Save File"_j, default_filename.string(), filters);
    if (!results) {
      rsl::error(results.error());
      rsl::ErrorDialog("Cannot save. " + results.error());
      return;
    }
    auto path = results->string();

    // Just autofill BBLM for now
    // .bblm1 .bblm2 should also be matched
    if (!results->extension().string().contains(".bblm") &&
        !path.ends_with(".pblm")) {
      path += ".bblm";
    }

    WriteBLM(m_grid.m_blm, path);
  }

private:
  BblmEditorPropertyGrid m_grid;
  BblmEditorTabSheet m_sheet;
  std::string m_path;
  lvl::AutoHistory<librii::egg::BLM> m_history;
};

} // namespace riistudio::frontend
