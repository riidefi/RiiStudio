#pragma once

#include <frontend/IEditor.hpp>
#include <frontend/legacy_editor/StudioWindow.hpp>
#include <frontend/widgets/PropertyEditorWidget.hpp>
#include <fstream>
#include <librii/arc/RARC.hpp>
#include <librii/szs/SZS.hpp>
#include <rsl/FsDialog.hpp>

namespace riistudio::frontend {

// Implements a single tab for now
class RarcEditorTabSheet : private PropertyEditorWidget {
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
  std::vector<std::string> TabTitles() override { return {"RARC"}; }
  bool Tab(int index) override {
    m_drawTab();
    return true;
  }

  std::function<void(void)> m_drawTab = nullptr;
};

class RarcEditor;

class RarcEditorPropertyGrid {
  friend class RarcEditor;

public:
  RarcEditorPropertyGrid() = default;
  void Draw(librii::RARC::ResourceArchive& arc, RarcEditor* editor);

private:
  void DrawTable(librii::RARC::ResourceArchive& arc, RarcEditor* editor);
  void DrawFlagsColumn(librii::RARC::ResourceArchive::Node& node,
                       RarcEditor* editor);
  void DrawContextMenu(librii::RARC::ResourceArchive::Node& node,
                       RarcEditor* editor);
  void DrawNameModal(RarcEditor* editor);

  std::optional<librii::RARC::ResourceArchive::Node> m_focused_node;
  std::string m_name_input;
  std::string m_original_name;
  bool m_flag_modal_opening;
  bool m_flag_modal_open;
  bool m_should_calc_changes;
};

class RarcEditor : public frontend::StudioWindow, public IEditor {
public:
  RarcEditor()
      : StudioWindow("RARC Editor: <unknown>", DockSetting::None), m_grid(),
        m_changes_made(), m_node_to_rename(), m_node_new_name(),
        m_files_to_insert(), m_folder_to_insert(), m_folder_to_create(),
        m_node_to_extract(), m_extract_path() {
    // setWindowFlag(ImGuiWindowFlags_MenuBar);
  }
  RarcEditor(const RarcEditor&) = delete;
  RarcEditor(RarcEditor&&) = delete;

  void draw_() override {
    setName("RARC Editor: " + m_path);
    m_sheet.Draw([&]() { m_grid.Draw(m_rarc, this); });
  }
  ImGuiID buildDock(ImGuiID root_id) override {
    // ImGui::DockBuilderDockWindow("Properties", root_id);
    return root_id;
  }

  std::string discordStatus() const override {
    return "Editing a ??? (RARC) file.";
  }

  void openFile(rsl::byte_view buf, std::string_view path) {
    Result<librii::RARC::ResourceArchive> arc;
    if (librii::szs::isDataYaz0Compressed(buf)) {
      std::span<u8> data;
      if (!librii::szs::decode(data, buf)) {
        rsl::ErrorDialogFmt("Failed to decompress RARC\n{}", arc.error());
        return;
      }
      arc = librii::RARC::LoadResourceArchive(data);
    } else {
      arc = librii::RARC::LoadResourceArchive(buf);
    }
    if (!arc) {
      rsl::ErrorDialogFmt("Failed to parse RARC\n{}", arc.error());
      return;
    }
    m_rarc = *arc;
    m_path = path;
  }

  void saveAs(std::string_view path) {
    auto barc = librii::RARC::SaveResourceArchive(m_rarc);
    if (!barc) {
      rsl::ErrorDialogFmt("Failed to save RARC\n{}", barc.error());
      return;
    }
    std::ofstream out(path.data(), std::ios::binary | std::ios::ate);
    out.write(reinterpret_cast<const char*>(barc->data()), barc->size());
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
    std::vector<std::string> filters{"??? (*.arc)", "*.arc"};
    auto results =
        rsl::SaveOneFile("Save File"_j, default_filename.string(), filters);
    if (!results) {
      rsl::ErrorDialog("No saving - No file selected");
      return;
    }
    auto path = results->string();

    if (!path.ends_with(".arc")) {
      path += ".arc";
    }

    saveAs(path);
  }

  void RenameNode(const librii::RARC::ResourceArchive::Node& node,
                  const std::string& new_name) {
    m_node_to_rename = node;
    m_node_new_name = new_name;
  }

  void InsertFiles(const std::vector<rsl::File>& files,
                   std::optional<s32> parent = std::nullopt) {
    if (parent)
      SetParentNode(*parent);
    m_files_to_insert = files;
  }

  void InsertFolder(const std::filesystem::path& folder,
                    std::optional<s32> parent = std::nullopt) {
    if (parent)
      SetParentNode(*parent);
    m_folder_to_insert = folder;
  }

  void CreateFolder(const std::string& folder,
                    std::optional<s32> parent = std::nullopt) {
    if (parent)
      SetParentNode(*parent);
    m_folder_to_create = folder;
  }

  void ExtractNodeTo(const librii::RARC::ResourceArchive::Node& node,
                     const std::filesystem::path& dst) {
    m_node_to_extract = node;
    m_extract_path = dst;
  }

  void SetParentNode(s32 parent) { m_insert_parent = parent; }

  void
  DeleteNodes(const std::vector<librii::RARC::ResourceArchive::Node>& nodes) {
    m_nodes_to_delete = nodes;
  }

  void ApplyUpdates() { reconstruct(); }

private:
  RarcEditorPropertyGrid m_grid;
  RarcEditorTabSheet m_sheet;
  std::string m_path;
  librii::RARC::ResourceArchive m_rarc;
  bool m_changes_made;

  //// Edit state

  // Parent id
  s32 m_insert_parent;

  // Rename
  std::optional<librii::RARC::ResourceArchive::Node> m_node_to_rename;
  std::string m_node_new_name;

  // Addition operations
  std::vector<rsl::File> m_files_to_insert;
  std::optional<std::filesystem::path> m_folder_to_insert;
  std::optional<std::string> m_folder_to_create;

  // Subtraction operations
  std::vector<librii::RARC::ResourceArchive::Node> m_nodes_to_delete;

  // Extract
  std::optional<librii::RARC::ResourceArchive::Node> m_node_to_extract;
  std::filesystem::path m_extract_path;

  Result<void> reconstruct();
};

} // namespace riistudio::frontend
