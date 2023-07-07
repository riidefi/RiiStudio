#include <frontend/widgets/ReflectedPropertyGrid.hpp>

#include "RarcEditor.hpp"
#include <rsl/Defer.hpp>
#include <vendor/fa5/IconsFontAwesome5.h>
#include <unordered_set>

namespace riistudio::frontend {

using namespace librii::RARC;

void RarcEditorPropertyGrid::Draw(ResourceArchive& rarc, RarcEditor* editor) {
  RSL_DEFER(editor->ApplyUpdates());

  constexpr auto dir_flags = ImGuiTreeNodeFlags_OpenOnArrow |
                             ImGuiTreeNodeFlags_OpenOnDoubleClick |
                             ImGuiTreeNodeFlags_SpanFullWidth;

  constexpr auto file_flags =
      ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanFullWidth;

  constexpr auto root_flags = dir_flags | ImGuiTreeNodeFlags_DefaultOpen;

  if (!m_flag_modal_open)
    m_focused_node = std::nullopt;

  if (ImGui::BeginTable("TABLE", 2,
                        ImGuiTableFlags_RowBg |
                            ImGuiTableFlags_NoSavedSettings)) {
    RSL_DEFER(ImGui::EndTable());

    ImGui::TableSetupScrollFreeze(2, 1);
    ImGui::TableSetupColumn("Node Tree");
    ImGui::TableSetupColumn("Flags", ImGuiTableColumnFlags_WidthFixed, 240.0f);
    ImGui::TableHeadersRow();

    std::vector<std::size_t> walk_stack;

    std::size_t row = 0;
    for (auto node = rarc.nodes.begin(); node != rarc.nodes.end(); row++) {
      // We don't render . and ..
      if (node->is_special_path()) {
        node++;
        continue;
      }


      ImGui::TableNextRow();
      ImGui::TableNextColumn();

      // Alternate row colors
      ImGui::TableSetBgColor(
          ImGuiTableBgTarget_RowBg0,
          ImGui::ColorConvertFloat4ToU32(
              row % 2 ? ImGui::GetStyleColorVec4(ImGuiCol_TableRowBgAlt)
                      : ImGui::GetStyleColorVec4(ImGuiCol_TableRowBg)));

      auto next_node = node + 1;

      if (node->is_folder()) {
        std::string awesome_name = std::format("{}    {}", (const char *)ICON_FA_FOLDER, node->name);
        auto flags = node->folder.parent == -1 ? root_flags : dir_flags;
        if (ImGui::TreeNodeEx(awesome_name.c_str(), flags)) {
          walk_stack.push_back(node->folder.sibling_next);
        } else {
          next_node = rarc.nodes.begin() + node->folder.sibling_next;
        }
      } else {
        std::string awesome_name =
            std::format("{}    {}", (const char*)ICON_FA_FILE, node->name);
        if (ImGui::TreeNodeEx(awesome_name.c_str(), file_flags))
          ImGui::TreePop();
      }

      DrawContextMenu(*node, editor);

      ImGui::TableNextColumn();

      DrawFlagsColumn(*node, editor);

      node = next_node;

      // Check for ended folders
      for (auto walk = walk_stack.begin(); walk != walk_stack.end();) {
        if (*walk == std::distance(rarc.nodes.begin(), node)) {
          // We assume rightfully that all future walks are also ended
          std::size_t ended_walks = std::distance(walk, walk_stack.end());
          for (int i = 0; i < ended_walks; i++) {
            walk_stack.pop_back();
            ImGui::TreePop();
          }
          break;
        }
        walk++;
      }
    }

    for (auto& walk : walk_stack) {
      ImGui::TreePop();
    }
  }

  DrawCreateModal(editor);
}

void RarcEditorPropertyGrid::DrawContextMenu(ResourceArchive::Node& node,
                                             RarcEditor* editor) {
  if (ImGui::BeginPopupContextItem()) {
    RSL_DEFER(ImGui::EndPopup());
    m_focused_node = node;
    if (node.is_folder()) {
      if (ImGui::MenuItem("Import Files..."_j)) {
        auto files = rsl::ReadManyFile("Import Path"_j, "", {"Files", "*.*"});
        if (!files)
          return;
        editor->InsertFiles(*files, node.id);
      }
      if (ImGui::MenuItem("Import Folder..."_j)) {
        auto folder = rsl::OpenFolder("Import Path"_j, "");
        if (!folder)
          return;
        editor->InsertFolder(*folder, node.id);
      }
      if (ImGui::MenuItem("Create Folder..."_j)) {
        m_flag_modal_opening = true;
        editor->SetParentNode(node.id);
      }
    }
    if (ImGui::MenuItem("Delete"_j)) {
      editor->DeleteNodes({node});
    }
    if (ImGui::MenuItem("Rename..."_j)) {
      std::cout << "Rename!\n";
    }
    if (ImGui::MenuItem("Extract..."_j)) {
      std::cout << "Extract!\n";
    }
    if (ImGui::MenuItem("Replace..."_j)) {
      std::cout << "Replace!\n";
    }
  }
}

void RarcEditorPropertyGrid::DrawCreateModal(RarcEditor* editor) {
  if (m_flag_modal_opening) {
    ImGui::OpenPopup("New Folder"_j);
  }

  auto flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove |
               ImGuiWindowFlags_AlwaysAutoResize;
  bool open = true;

  if (ImGui::BeginPopupModal("New Folder"_j, &open, flags)) {
    RSL_DEFER(ImGui::EndPopup());

    m_flag_modal_opening = false;
    m_flag_modal_open = true;

    char input_buf[128]{};
    snprintf(input_buf, sizeof(input_buf), "%s", m_new_folder_name.c_str());

    if (ImGui::InputText("Name", input_buf, sizeof(input_buf),
                         ImGuiInputTextFlags_CharsNoBlank |
                             ImGuiInputTextFlags_AutoSelectAll)) {
      m_new_folder_name = input_buf;
    }

    if (m_new_folder_name == "") {
      // Then, we get the bounding box of the last item
      ImRect bb = {ImGui::GetItemRectMin(), ImGui::GetItemRectMax()};
      bb.Max.x -= 36; // Adjust for the name label

      // Then we use the ImGui draw list to draw a custom border
      ImDrawList* draw_list = ImGui::GetWindowDrawList();
      draw_list->AddRect(bb.Min, bb.Max, IM_COL32(255, 0, 0, 255)); // RGBA
    }

	if (!open) {
      ImGui::CloseCurrentPopup();
      m_flag_modal_open = false;
      return;
	}

    auto* enter_key = ImGui::GetKeyData(ImGuiKey_Enter);
    if (enter_key->Down && m_new_folder_name != "") {
      ImGui::CloseCurrentPopup();
      editor->CreateFolder(m_new_folder_name);
      m_flag_modal_open = false;
    }
  }
}

void RarcEditorPropertyGrid::DrawFlagsColumn(ResourceArchive::Node& node,
                                             RarcEditor* editor) {
  std::vector<std::string> modes = {"Preload to MRAM", "Preload to ARAM",
                                    "Load from DVD"};

  int mode_index = 0;
  if ((node.flags & ResourceAttribute::PRELOAD_TO_ARAM))
    mode_index = 1;
  else if ((node.flags & ResourceAttribute::LOAD_FROM_DVD))
    mode_index = 2;

  ImGui::PushID(std::addressof(node));
  if (node.is_folder()) {
    ImGui::Text(node.folder.parent == -1 ? "Root" : "Directory");
  } else {
    ImGuiStyle& style = ImGui::GetStyle();
    float orig_padding_y = style.FramePadding.y;
    float orig_spacing_y = style.ItemSpacing.y;

    style.FramePadding.y = 0;
    style.ItemSpacing.y = 0;

    if (ImGui::BeginCombo("Load State", modes[mode_index].c_str(),
                          ImGuiComboFlags_HeightSmall)) {
      for (int i = 0; i < modes.size(); ++i) {
        bool is_selected = i == mode_index;
        if (ImGui::Selectable(modes[i].c_str(), &is_selected))
          mode_index = i;
        if (is_selected) {
          if (mode_index == 0) {
            node.flags |= ResourceAttribute::PRELOAD_TO_MRAM;
            node.flags &= ~(ResourceAttribute::PRELOAD_TO_ARAM |
                            ResourceAttribute::LOAD_FROM_DVD);
          }

          if (mode_index == 1) {
            node.flags |= ResourceAttribute::PRELOAD_TO_ARAM;
            node.flags &= ~(ResourceAttribute::PRELOAD_TO_MRAM |
                            ResourceAttribute::LOAD_FROM_DVD);
          }

          if (mode_index == 2) {
            node.flags |= ResourceAttribute::LOAD_FROM_DVD;
            node.flags &= ~(ResourceAttribute::PRELOAD_TO_MRAM |
                            ResourceAttribute::PRELOAD_TO_ARAM);
          }
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();

      style.FramePadding.y = orig_padding_y;
      style.ItemSpacing.y = orig_spacing_y;
    }
  }
  ImGui::PopID();
}

static void
get_sorted_directory_list_r(const std::filesystem::path& path,
                            std::vector<std::filesystem::path>& out) {
  std::vector<std::filesystem::path> dirs;
  for (auto&& it : std::filesystem::directory_iterator{path}) {
    if (it.is_directory())
      dirs.push_back(it.path());
    else
      out.push_back(it.path());
  }
  for (auto& dir : dirs) {
    out.push_back(dir);
    get_sorted_directory_list_r(dir, out);
  }
}

static bool insertFiles(ResourceArchive& rarc,
                        std::optional<ResourceArchive::Node> parent,
                        std::vector<rsl::File>& files) {
  if (files.size() == 0)
    return false;

  if (!parent) {
    files.clear();
    return false;
  }

  std::vector<ResourceArchive::Node> new_nodes;
  for (auto& file : files) {
    auto file_name = file.path.filename().string();
    std::transform(file_name.begin(), file_name.end(), file_name.begin(),
                   [](u8 ch) { return std::tolower(ch); });
    ResourceArchive::Node node = {.id = 0,
                                  .flags = ResourceAttribute::FILE |
                                           ResourceAttribute::PRELOAD_TO_MRAM,
                                  .name = file_name,
                                  .data = file.data};
    new_nodes.push_back(node);
  }

  int parent_index =
      std::distance(rarc.nodes.begin(),
                    std::find(rarc.nodes.begin(), rarc.nodes.end(), *parent));

  // Manual sort is probably better because we can dodge DFS nonsense
  // effectively Even with O(n^2) because there typically aren't many nodes
  // anyway.
  for (auto new_node = new_nodes.begin(); new_node != new_nodes.end();
       new_node++) {
    auto dir_files_start = rarc.nodes.begin() + parent_index + 1;
    auto dir_files_end = rarc.nodes.begin() + parent->folder.sibling_next +
                         std::distance(new_nodes.begin(), new_node);
    for (auto child = dir_files_start; /*nothing*/; child++) {
      // Always insert at the very end (alphabetically last)
      if (child == dir_files_end) {
        rarc.nodes.insert(child, *new_node);
        break;
      }
      if (child->is_folder()) {
        // Never insert before the special dirs
        if (child->is_special_path())
          continue;
        // Always insert before the regular dirs
        rarc.nodes.insert(child, *new_node);
        break;
      }
      if (new_node->name < child->name) {
        rarc.nodes.insert(child, *new_node);
        break;
      }
    }
  }

  for (auto& node : rarc.nodes) {
    if (!node.is_folder())
      continue;
    if (node.folder.sibling_next <= parent_index)
      continue;
    node.folder.sibling_next += files.size();
  }

  files.clear();
  return true;
}

static bool insertFolder(ResourceArchive& rarc,
                         std::optional<ResourceArchive::Node> parent,
                         std::optional<std::filesystem::path>& folder) {
  if (!folder)
    return false;

  if (!parent) {
    folder = std::nullopt;
    return false;
  }

  // Generate an archive so we can steal the DFS structure.
  auto tmp_rarc = librii::RARC::Create(*folder);
  if (!tmp_rarc) {
    folder = std::nullopt;
    return false;
  }

  int parent_index =
      std::distance(rarc.nodes.begin(),
                    std::find(rarc.nodes.begin(), rarc.nodes.end(), *parent));

  auto folder_name = folder->filename().string();
  std::transform(folder_name.begin(), folder_name.end(), folder_name.begin(),
                 [](char ch) { return std::tolower(ch); });

  auto insert_index = rarc.nodes.size();
  auto dir_files_start = rarc.nodes.begin() + parent_index + 1;
  auto dir_files_end = rarc.nodes.begin() + parent->folder.sibling_next;
  for (auto child = dir_files_start; /*nothing*/;) {
    if (child == dir_files_end) {
      // Always insert at the very end (alphabetically last)
      insert_index = std::distance(rarc.nodes.begin(), child);
      for (auto& new_node : tmp_rarc->nodes) {
        if (new_node.is_folder())
          new_node.folder.sibling_next += insert_index;
      }
      rarc.nodes.insert(child, tmp_rarc->nodes.begin(), tmp_rarc->nodes.end());
      break;
    }

    // Never insert in the files
    if (!child->is_folder()) {
      child++;
      continue;
    }

    // Never insert before the special dirs
    if (child->is_special_path()) {
      child++;
      continue;
    }

    // We are attempting to insert the entire
    // RARC fs as a new folder here
    if (tmp_rarc->nodes[0].name < child->name) {
      insert_index = std::distance(rarc.nodes.begin(), child);
      for (auto& new_node : tmp_rarc->nodes) {
        if (new_node.is_folder())
          new_node.folder.sibling_next += insert_index;
        // new_node.folder.sibling_next -= tmp_rarc->nodes.size();  // Adjust
        // for the loop later.
      }
      rarc.nodes.insert(child, tmp_rarc->nodes.begin(), tmp_rarc->nodes.end());
      break;
    }

    // Skip to the next dir (DFS means we jump subnodes)
    child = rarc.nodes.begin() + child->folder.sibling_next;
  }

  for (auto node = rarc.nodes.begin(); node != rarc.nodes.end();) {
    auto index = std::distance(rarc.nodes.begin(), node);
    // Skip files and the special dirs (they get recalculated later)
    if (!node->is_folder() || node->is_special_path()) {
      node++;
      continue;
    }
    if (index == insert_index) { // Skip inserted dir (already adjusted)
      node = rarc.nodes.begin() + node->folder.sibling_next;
      continue;
    }
    if (node->folder.sibling_next <
        insert_index) { // Skip dirs that come before insertion by span alone
      node = rarc.nodes.begin() + node->folder.sibling_next;
      continue;
    }
    // This is a directory adjacent but not beyond the insertion, ignore.
    if (node->folder.parent >= parent->id && index < insert_index) {
      node = rarc.nodes.begin() + node->folder.sibling_next;
      continue;
    }
    node->folder.sibling_next += tmp_rarc->nodes.size();
    node++;
  }

  folder = std::nullopt;
  return true;
}

static bool createFolder(ResourceArchive& rarc,
                         std::optional<ResourceArchive::Node> parent,
                         std::optional<std::string>& folder) {
  if (!folder)
    return false;

  if (!parent) {
    folder = std::nullopt;
    return false;
  }

  int parent_index =
      std::distance(rarc.nodes.begin(),
                    std::find(rarc.nodes.begin(), rarc.nodes.end(), *parent));

  auto folder_name = *folder;
  std::transform(folder_name.begin(), folder_name.end(), folder_name.begin(),
                 [](char ch) { return std::tolower(ch); });

  std::vector<ResourceArchive::Node> folder_nodes = {
      {.id = 0,
       .flags = ResourceAttribute::DIRECTORY,
       .name = folder_name,
       .folder = {.parent = parent->id, .sibling_next = 0}},
      {.id = 0,
       .flags = ResourceAttribute::DIRECTORY,
       .name = ".",
       .folder = {.parent = parent->id, .sibling_next = 0}},
      {.id = 0,
       .flags = ResourceAttribute::DIRECTORY,
       .name = "..",
       .folder = {.parent = parent->folder.parent,
                  .sibling_next = parent->folder.sibling_next}},
  };

  auto insert_index = rarc.nodes.size();
  auto dir_files_start = rarc.nodes.begin() + parent_index + 1;
  auto dir_files_end = rarc.nodes.begin() + parent->folder.sibling_next;
  for (auto child = dir_files_start; /*nothing*/;) {
    if (child == dir_files_end) {
      // Always insert at the very end (alphabetically last)
      insert_index = std::distance(rarc.nodes.begin(), child);
      folder_nodes[0].folder.sibling_next = insert_index + 3;
      folder_nodes[1].folder.sibling_next = insert_index + 3;
      rarc.nodes.insert(child, folder_nodes.begin(), folder_nodes.end());
      break;
    }

    // Never insert in the files
    if (!child->is_folder()) {
      child++;
      continue;
    }

    // Never insert before the special dirs
    if (child->is_special_path()) {
      child++;
      continue;
    }

    // We are attempting to insert the entire
    // RARC fs as a new folder here
    if (folder_name < child->name) {
      insert_index = std::distance(rarc.nodes.begin(), child);
      folder_nodes[0].folder.sibling_next = insert_index + 3;
      folder_nodes[1].folder.sibling_next = insert_index + 3;
      rarc.nodes.insert(child, folder_nodes.begin(), folder_nodes.end());
      break;
    }

    // Skip to the next dir (DFS means we jump subnodes)
    child = rarc.nodes.begin() + child->folder.sibling_next;
  }

  for (auto node = rarc.nodes.begin(); node != rarc.nodes.end();) {
    auto index = std::distance(rarc.nodes.begin(), node);
    // Skip files and the special dirs (they get recalculated later)
    if (!node->is_folder() || node->is_special_path()) {
      node++;
      continue;
    }
    if (index == insert_index) { // Skip inserted dir (already adjusted)
      node = rarc.nodes.begin() + node->folder.sibling_next;
      continue;
    }
    if (node->folder.sibling_next <
        insert_index) { // Skip dirs that come before insertion by span alone
      node = rarc.nodes.begin() + node->folder.sibling_next;
      continue;
    }
    // This is a directory adjacent but not beyond the insertion, ignore.
    if (node->folder.parent >= parent->id && index < insert_index) {
      node = rarc.nodes.begin() + node->folder.sibling_next;
      continue;
    }
    node->folder.sibling_next += 3;
    node++;
  }

  folder = std::nullopt;
  return true;
}

static bool deleteNodes(ResourceArchive& rarc,
                        std::vector<ResourceArchive::Node>& nodes) {
  if (nodes.size() == 0)
    return false;

  for (auto& to_delete : nodes) {
    for (int i = 0; i < rarc.nodes.size(); i++) {
      if (to_delete == rarc.nodes[i]) {
        if (!to_delete.is_folder()) {
          ResourceArchive::Node parent;
          int parent_index = 0;
          for (parent_index = i; parent_index >= 0; parent_index--) {
            parent = rarc.nodes[parent_index];
            if (!parent.is_folder() || parent.is_special_path())
              continue;
            // This folder should be the deepest parent
            if (parent.folder.sibling_next > i)
              break;
          }
          auto deleted_at = std::distance(
              rarc.nodes.begin(), rarc.nodes.erase(rarc.nodes.begin() + i));

          for (auto& node : rarc.nodes) {
            if (!node.is_folder())
              continue;
            if (node.folder.sibling_next <= deleted_at)
              continue;
            node.folder.sibling_next--;
          }
        } else {
          auto begin = rarc.nodes.begin() + i;
          auto end = rarc.nodes.begin() + to_delete.folder.sibling_next;
          auto size = std::distance(begin, end);

          auto deleted_at =
              std::distance(rarc.nodes.begin(), rarc.nodes.erase(begin, end));

          for (auto& node : rarc.nodes) {
            if (!node.is_folder())
              continue;
            if (node.folder.sibling_next <= deleted_at)
              continue;
            if (node.folder.parent > to_delete.id)
              node.folder.parent -= 1;
            node.folder.sibling_next -= size;
          }
        }
      }
    }
  }

  nodes.clear();
  return true;
}

Result<void> RarcEditor::reconstruct() {
  // Start by inserting the files into the folder list,
  // sorted by name
  std::optional<ResourceArchive::Node> parent_node;
  std::size_t parent_index;
  for (std::size_t i = 0; i < m_rarc.nodes.size(); ++i) {
    auto& node = m_rarc.nodes[i];
    if (node.is_folder() && node.id == m_insert_parent) {
      parent_node = node;
      parent_index = i;
      break;
    }
  }

  bool changes_applied = false;

  // Delete things first to potentially improve performance...
  // Realistically only one of these will be run at a time
  // So we can just recalculate the IDs once at the end.
  changes_applied |= deleteNodes(m_rarc, m_nodes_to_delete);
  changes_applied |= createFolder(m_rarc, parent_node, m_folder_to_create);
  changes_applied |= insertFiles(m_rarc, parent_node, m_files_to_insert);
  changes_applied |= insertFolder(m_rarc, parent_node, m_folder_to_insert);

  if (changes_applied) {
    m_changes_made = true;
    librii::RARC::RecalculateArchiveIDs(m_rarc);
  }
  return {};
}

} // namespace riistudio::frontend
