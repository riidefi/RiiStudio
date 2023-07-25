#pragma once

#include <frontend/IEditor.hpp>
#include <frontend/legacy_editor/StudioWindow.hpp>
#include <frontend/widgets/PropertyEditorWidget.hpp>
#include <fstream>
#include <librii/rarc/RARC.hpp>
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

enum class TriState {
  ST_FALSE,
  ST_TRUE,
  ST_INDETERMINATE,
};

enum class ModalState {
  M_CLOSED,
  M_OPENING,
  M_OPEN,
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
  void DrawSizeColumn(librii::RARC::ResourceArchive::Node& node,
                      RarcEditor* editor);
  void DrawContextMenu(librii::RARC::ResourceArchive::Node& node,
                       RarcEditor* editor);
  void DrawNameModal(RarcEditor* editor);
  void DrawOperationModal(RarcEditor* editor);

  ImGuiTextFilter m_model_filter;
  std::optional<librii::RARC::ResourceArchive::Node> m_focused_node;
  std::string m_name_input;
  std::string m_original_name;
  ModalState m_context_modal_state;
  ModalState m_operation_modal_state;
  TriState m_operation_state;
  bool m_should_calc_changes;
};

class RarcEditor : public frontend::StudioWindow, public IEditor {
public:
  RarcEditor() : StudioWindow("RARC Editor: <unknown>", DockSetting::None) {}
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
    return "Editing a Resource Archive.";
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
    std::vector<std::string> filters{
        "Compressed Archive (*.szs)", "*.szs", "Archive (*.arc)", "*.arc",
        "Archive (*.carc)",           "*.carc"};
    auto results =
        rsl::SaveOneFile("Save File"_j, default_filename.string(), filters);
    if (!results) {
      rsl::ErrorDialog("No saving - No file selected");
      return;
    }
    auto path = results->string();

    if (!path.ends_with(".arc") && !path.ends_with(".carc") &&
        !path.ends_with(".szs")) {
      path += ".arc";
    }

    saveAs(path);
  }

  std::size_t GetNodeSize(librii::RARC::ResourceArchive::Node& node) {
    if (node.is_folder()) {
      return node.folder.sibling_next -
             std::distance(
                 m_rarc.nodes.begin(),
                 std::find(m_rarc.nodes.begin(), m_rarc.nodes.end(), node)) -
             1;
    }
    return node.data.size();
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

  void ReplaceNodeWith(const librii::RARC::ResourceArchive::Node& node,
                       const std::filesystem::path& dst) {
    m_node_to_replace = node;
    m_replace_path = dst;
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

  //// Edit state
  bool m_changes_made;

  // Parent id
  s32 m_insert_parent;

  // Rename
  std::optional<librii::RARC::ResourceArchive::Node> m_node_to_rename =
      std::nullopt;
  std::string m_node_new_name = "";

  // Addition operations
  std::vector<rsl::File> m_files_to_insert = {};
  std::optional<std::filesystem::path> m_folder_to_insert = std::nullopt;
  std::optional<std::string> m_folder_to_create = std::nullopt;

  // Subtraction operations
  std::vector<librii::RARC::ResourceArchive::Node> m_nodes_to_delete = {};

  // Extract
  std::optional<librii::RARC::ResourceArchive::Node> m_node_to_extract =
      std::nullopt;
  std::filesystem::path m_extract_path = ".";

  // Replace
  std::optional<librii::RARC::ResourceArchive::Node> m_node_to_replace =
      std::nullopt;
  std::filesystem::path m_replace_path = ".";

  Result<void> reconstruct();
};

} // namespace riistudio::frontend
