#include <frontend/widgets/ReflectedPropertyGrid.hpp>

#include "RarcEditor.hpp"
#include <rsl/Defer.hpp>
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
    ImGui::TableSetupColumn("Node");
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
        auto flags = node->folder.parent == -1 ? root_flags : dir_flags;
        if (ImGui::TreeNodeEx(node->name.c_str(), flags)) {
          walk_stack.push_back(node->folder.sibling_next);
        } else {
          next_node = rarc.nodes.begin() + node->folder.sibling_next;
        }
      } else {
        if (ImGui::TreeNodeEx(node->name.c_str(), file_flags))
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

    /*auto modal_id = ImGui::GetID("Modify Flags");

    if (m_flag_modal_opening) {
      ImGui::OpenPopup(modal_id);
      m_flag_modal_opening = false;
    }

    if (ImGui::BeginPopupEx(modal_id, ImGuiWindowFlags_Modal |
                                          ImGuiWindowFlags_NoResize |
                                          ImGuiWindowFlags_AlwaysAutoResize)) {
      m_flag_modal_open = true;
      if ((m_focused_node->flags & ResourceAttribute::PRELOAD_TO_MRAM))
        m_load_state = 0;
      else if ((m_focused_node->flags & ResourceAttribute::PRELOAD_TO_ARAM))
        m_load_state = 1;
      else
        m_load_state = 2;

      if (ImGui::RadioButton("Preload to MRAM", &m_load_state, 0)) {
        m_focused_node->flags |= ResourceAttribute::PRELOAD_TO_MRAM;
        m_focused_node->flags &= ~(ResourceAttribute::PRELOAD_TO_ARAM |
                                   ResourceAttribute::LOAD_FROM_DVD);
      }

      if (ImGui::RadioButton("Preload to ARAM", &m_load_state, 1)) {
        m_focused_node->flags |= ResourceAttribute::PRELOAD_TO_ARAM;
        m_focused_node->flags &= ~(ResourceAttribute::PRELOAD_TO_MRAM |
                                   ResourceAttribute::LOAD_FROM_DVD);
      }

      if (ImGui::RadioButton("Load from DVD", &m_load_state, 2)) {
        m_focused_node->flags |= ResourceAttribute::LOAD_FROM_DVD;
        m_focused_node->flags &= ~(ResourceAttribute::PRELOAD_TO_MRAM |
                                   ResourceAttribute::PRELOAD_TO_ARAM);
      }

      if (ImGui::Button("Close")) {
        ImGui::CloseCurrentPopup();
        m_flag_modal_open = false;
      }
      ImGui::EndPopup();
    }*/
  }
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
        editor->InsertFolders({*folder}, node.id);
      }
      if (ImGui::MenuItem("Create Folder..."_j)) {
        auto dir = rsl::OpenFolder("Create Folder..."_j, "", false);
        if (!dir)
          return;
        editor->CreateFolder(*dir, node.id);
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

template <class _RanIt> static constexpr void fsSort(_RanIt begin, _RanIt end) {
  std::sort(begin, end,
            [&](ResourceArchive::Node lhs, ResourceArchive::Node rhs) {
              if (lhs.name == rhs.name)
                return false;

              if (lhs.name == ".")
                return true;

              if (rhs.name == ".")
                return false;

              if (lhs.name == "..")
                return true;

              if (rhs.name == "..")
                return false;

              bool is_lhs_folder = lhs.is_folder();
              bool is_rhs_folder = rhs.is_folder();

              if (!is_lhs_folder && is_rhs_folder)
                return true;

              if (is_lhs_folder && !is_rhs_folder)
                return false;

              return lhs.name < rhs.name;
            });
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


  // Manual sort is probably better because we can dodge DFS nonsense effectively
  // Even with O(n^2) because there typically aren't many nodes anyway.
  for (auto new_node = new_nodes.begin(); new_node != new_nodes.end(); new_node++) {
	auto dir_files_start = rarc.nodes.begin() + parent_index + 1;
	auto dir_files_end = rarc.nodes.begin() + parent->folder.sibling_next + std::distance(new_nodes.begin(), new_node);
    for (auto child = dir_files_start; child != dir_files_end; child++) {
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
                         std::optional<std::filesystem::path> folder) {
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



  //ResourceArchive::Node new_folder = {
  //    .id = 0,
  //    .flags = ResourceAttribute::DIRECTORY,
  //    .name = folder_name,
  //    .folder = {.parent = parent->id, .sibling_next = 0}};

  //{
  //  auto dir_last = rarc.nodes.begin() + parent->folder.sibling_next - 1;
  //  rarc.nodes.insert(dir_last, new_folder);
  //}

  //auto dir_start = rarc.nodes.begin() + parent_index + 1;
  //auto dir_end = rarc.nodes.begin() + parent->folder.sibling_next + 1;

  //// Sort the directory after inserting the new empty directory
  //fsSort(dir_start, dir_end);

  //auto this_iter = std::find(dir_start, dir_end, new_folder);

  //std::vector<std::filesystem::path> flat_sorted_list;
  //get_sorted_directory_list_r(*folder, flat_sorted_list);

  /*for (auto& node : rarc.nodes) {
    if (!node.is_folder())
      continue;
    if (node.folder.parent > parent->folder.parent)
      continue;
    if (node.folder.parent == parent->folder.parent && node.id < parent->id)
      continue;
    node.folder.sibling_next += files.size();
  }

  files.clear();*/
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
          auto deleted_at = std::distance(rarc.nodes.begin(), rarc.nodes.erase(rarc.nodes.begin() + i));

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

          auto deleted_at = std::distance(rarc.nodes.begin(), rarc.nodes.erase(begin, end));

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
  changes_applied |= insertFiles(m_rarc, parent_node, m_files_to_insert);
  changes_applied |= deleteNodes(m_rarc, m_nodes_to_delete);

  if (changes_applied) {
    m_changes_made = true;
    librii::RARC::RecalculateArchiveIDs(m_rarc);
  }
  return {};
}

} // namespace riistudio::frontend
