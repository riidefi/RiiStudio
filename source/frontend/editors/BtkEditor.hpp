#pragma once

#include <frontend/IEditor.hpp>
#include <frontend/legacy_editor/StudioWindow.hpp>
#include <frontend/widgets/AutoHistory.hpp>
#include <frontend/widgets/PropertyEditorWidget.hpp>
#include <librii/j3d/BinaryBTK.hpp>
#include <rsl/FsDialog.hpp>

namespace riistudio::frontend {

// Implements a single tab for now
class BtkEditorTabSheet {
public:
  void Draw(std::function<void(void)> draw) {
    std::vector<std::string> titles{
        "BTK",
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

class BtkEditorPropertyGrid {
public:
  void Draw(librii::j3d::BTK& btk);
};

class BtkEditor : public frontend::StudioWindow, public IEditor {
public:
  BtkEditor() : StudioWindow("BTK Editor: <unknown>", DockSetting::None) {
    // setWindowFlag(ImGuiWindowFlags_MenuBar);
  }
  BtkEditor(const BtkEditor&) = delete;
  BtkEditor(BtkEditor&&) = delete;

  void draw_() override {
    setName("BTK Editor: " + m_path);
    m_sheet.Draw([&]() { m_grid.Draw(m_btk); });
    m_history.update(m_btk);
  }
  ImGuiID buildDock(ImGuiID root_id) override {
    // ImGui::DockBuilderDockWindow("Properties", root_id);
    return root_id;
  }

  std::string discordStatus() const override {
    return "Editing a ??? (.btk) file.";
  }

  void openFile(std::span<const u8> buf, std::string_view path) {
    auto btk = librii::j3d::BTK::fromMemory(buf, path);
    if (!btk) {
      rsl::ErrorDialogFmt("Failed to parse BTK\n{}", btk.error());
      return;
    }
    m_btk = *btk;
    m_path = path;
  }
  void saveAs(std::string_view path) {
    rsl::ErrorDialog("BTK: File saving is not supported");
  }

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
    std::vector<std::string> filters{"??? (*.btk)", "*.btk"};
    auto results =
        rsl::SaveOneFile("Save File"_j, default_filename.string(), filters);
    if (!results) {
      rsl::ErrorDialog("No saving - No file selected");
      return;
    }
    auto path = results->string();

    if (!path.ends_with(".btk")) {
      path += ".btk";
    }

    saveAs(path);
  }

private:
  BtkEditorPropertyGrid m_grid;
  BtkEditorTabSheet m_sheet;
  std::string m_path;
  librii::j3d::BTK m_btk;
  lvl::AutoHistory<librii::j3d::BTK> m_history;
};

} // namespace riistudio::frontend
