#include <frontend/widgets/ReflectedPropertyGrid.hpp>
#include <frontend/widgets/utilities.hpp>

#include "RarcEditor.hpp"
#include <rsl/Defer.hpp>
#include <unordered_set>
#include <vendor/fa5/IconsFontAwesome5.h>

namespace riistudio::frontend {

using namespace librii::RARC;

struct ExtensionInfo {
  std::string m_category;
  std::string m_unicode_icon;
  std::optional<ImVec4> m_icon_color;
};

static ExtensionInfo s_animation_info = {"Animation Data",
                                         (const char*)ICON_FA_BEZIER_CURVE,
                                         {{0.8, 0.8, 0.2, 1.0}}};
static ExtensionInfo s_archive_info = {
    "Archive", (const char*)ICON_FA_ARCHIVE, {{0.7, 0.2, 0.2, 1.0}}};
static ExtensionInfo s_collision_info = {
    "Collision Data", (const char*)ICON_FA_CUBE, {{0.9, 0.5, 0.2, 1.0}}};
static ExtensionInfo s_compressed_info = {
    "Compressed Data", (const char*)ICON_FA_COMPRESS, {{0.7, 0.8, 0.2, 1.0}}};
static ExtensionInfo s_environment_info = {
    "Environment Data", (const char*)ICON_FA_CLOUD_SUN, {{0.4, 0.6, 0.9, 1.0}}};
static ExtensionInfo s_font_info = {"Font Data", (const char*)ICON_FA_PEN, {}};
static ExtensionInfo s_input_info = {
    "Input Data", (const char*)ICON_FA_GAMEPAD, {}};
static ExtensionInfo s_layout_info = {
    "Layout Data", (const char*)ICON_FA_MAP, {}};
static ExtensionInfo s_material_info = {
    "Material Data", (const char*)ICON_FA_PAINT_BRUSH, {{0.7, 0.3, 0.8, 1.0}}};
static ExtensionInfo s_message_info = {
    "Message Data", (const char*)ICON_FA_QUOTE_RIGHT, {}};
static ExtensionInfo s_model_info = {
    "Model Data", (const char*)ICON_FA_DRAW_POLYGON, {{0.3, 0.8, 0.3, 1.0}}};
static ExtensionInfo s_movie_info = {
    "Movie Data", (const char*)ICON_FA_VIDEO, {{0.9, 0.3, 0.3, 1.0}}};
static ExtensionInfo s_particle_info = {
    "Particle Data", (const char*)ICON_FA_FIRE, {{0.9, 0.7, 0.2, 1.0}}};
static ExtensionInfo s_script_info = {
    "Script Data", (const char*)ICON_FA_CODE, {{0.4, 0.9, 0.4, 1.0}}};
static ExtensionInfo s_sound_info = {
    "Sound Data", (const char*)ICON_FA_MUSIC, {{0.4, 0.5, 0.8, 1.0}}};
static ExtensionInfo s_texture_info = {
    "Texture Data", (const char*)ICON_FA_IMAGE, {{0.7, 0.3, 0.8, 1.0}}};
static ExtensionInfo s_track_info = {
    "Track Data", (const char*)ICON_FA_MOTORCYCLE, {{0.8, 0.5, 0.3, 1.0}}};

static std::unordered_map<std::string, ExtensionInfo> s_extension_map = {
    {"bck", s_animation_info},     {"blk", s_animation_info},
    {"bpk", s_animation_info},     {"brk", s_animation_info},
    {"btk", s_animation_info},     {"btp", s_animation_info},
    {"bva", s_animation_info},     {"arc", s_archive_info},
    {"carc", s_compressed_info},   {"u8", s_archive_info},
    {"col", s_collision_info},     {"kcl", s_collision_info},
    {"szs", s_compressed_info},    {"bdof", s_environment_info},
    {"bfg", s_environment_info},   {"blight", s_environment_info},
    {"blmap", s_environment_info}, {"pblm", s_environment_info},
    {"bin", s_environment_info},   {"ral", s_environment_info},
    {"ymp", s_environment_info},   {"bfn", s_font_info},
    {"pad", s_input_info},         {"blo", s_layout_info},
    {"brlyt", s_layout_info},      {"brlan", s_layout_info},
    {"brctr", s_layout_info},      {"bmt", s_material_info},
    {"bmg", s_message_info},       {"bdl", s_model_info},
    {"bmd", s_model_info},         {"brres", s_model_info},
    {"dae", s_model_info},         {"fbx", s_model_info},
    {"3ds", s_model_info},         {"thp", s_movie_info},
    {"jpa", s_particle_info},      {"sb", s_script_info},
    {"bas", s_sound_info},         {"bms", s_sound_info},
    {"com", s_sound_info},         {"scom", s_sound_info},
    {"aw", s_sound_info},          {"afc", s_sound_info},
    {"adp", s_sound_info},         {"ws", s_sound_info},
    {"bnk", s_sound_info},         {"bld", s_sound_info},
    {"bst", s_sound_info},         {"bti", s_texture_info},
    {"bmp", s_texture_info},       {"kmp", s_track_info},
};

void RarcEditorPropertyGrid::Draw(ResourceArchive& rarc, RarcEditor* editor) {
  RSL_DEFER(editor->ApplyUpdates());

  constexpr auto dir_flags = ImGuiTreeNodeFlags_OpenOnArrow |
                             ImGuiTreeNodeFlags_OpenOnDoubleClick |
                             ImGuiTreeNodeFlags_SpanFullWidth;

  constexpr auto file_flags =
      ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanFullWidth;

  constexpr auto root_flags = dir_flags | ImGuiTreeNodeFlags_DefaultOpen;

  if (m_context_modal_state == ModalState::M_CLOSED &&
      m_operation_modal_state == ModalState::M_CLOSED)
    m_focused_node = std::nullopt;

  ImGui::Text("Search");
  ImGui::SameLine();
  m_model_filter.Draw();

  if (ImGui::BeginTable("TABLE", 3,
                        ImGuiTableFlags_RowBg |
                            ImGuiTableFlags_NoSavedSettings)) {
    RSL_DEFER(ImGui::EndTable());

    ImGui::TableSetupScrollFreeze(3, 1);
    ImGui::TableSetupColumn("Node Tree");
    ImGui::TableSetupColumn("Flags", ImGuiTableColumnFlags_WidthFixed, 240.0f);
    ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 50.0f);
    ImGui::TableHeadersRow();

    std::vector<std::size_t> walk_stack;

    std::size_t row = 0;
    for (auto node = rarc.nodes.begin(); node != rarc.nodes.end(); row++) {
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

      if (!m_model_filter.PassFilter(node->name.c_str())) {
        node++;
        continue;
      }

      ExtensionInfo extension_info;
      {
        std::string extension = node->name.substr(node->name.find(".") + 1);
        if (node->is_folder()) {
          extension_info = {"Folder", (const char*)ICON_FA_FOLDER};
        } else if (s_extension_map.contains(extension)) {
          extension_info = s_extension_map[extension];
        } else {
          extension_info = {"All Files", (const char*)ICON_FA_FILE};
        }
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

      ImGui::PushID(node->name.c_str());
      {
        auto icon_color = extension_info.m_icon_color;
        if (icon_color)
          ImGui::PushStyleColor(ImGuiCol_Text, *icon_color);

        if (node->is_folder()) {
          auto flags = node->folder.parent == -1 ? root_flags : dir_flags;
          if (ImGui::TreeNodeEx(extension_info.m_unicode_icon.c_str(), flags)) {
            walk_stack.push_back(node->folder.sibling_next);
          } else {
            next_node = rarc.nodes.begin() + node->folder.sibling_next;
          }
          if (icon_color)
            ImGui::PopStyleColor();
          ImGui::SameLine();
          ImGui::Text("%s", node->name.c_str());
        } else {
          if (ImGui::TreeNodeEx(extension_info.m_unicode_icon.c_str(),
                                file_flags))
            ImGui::TreePop();
          if (icon_color)
            ImGui::PopStyleColor();
          ImGui::SameLine();
          ImGui::Text("%s", node->name.c_str());
        }
      }
      ImGui::PopID();

      DrawContextMenu(*node, editor);

      ImGui::TableNextColumn();

      DrawFlagsColumn(*node, editor);

      ImGui::TableNextColumn();

      DrawSizeColumn(*node, editor);

      node = next_node;
    }

    for (auto& walk : walk_stack) {
      ImGui::TreePop();
    }
  }

  DrawNameModal(editor);
  DrawOperationModal(editor);
}

void RarcEditorPropertyGrid::DrawContextMenu(ResourceArchive::Node& node,
                                             RarcEditor* editor) {
  if (ImGui::BeginPopupContextItem(node.name.c_str())) {
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
        m_context_modal_state = ModalState::M_OPENING;
        m_name_input = "";
        m_original_name = "";
        editor->SetParentNode(node.id);
      }
    }
    if (ImGui::MenuItem("Delete"_j)) {
      editor->DeleteNodes({node});
    }
    if (ImGui::MenuItem("Rename..."_j)) {
      m_context_modal_state = ModalState::M_OPENING;
      m_name_input = node.name;
      m_original_name = node.name;
      m_focused_node = node;
    }
    if (ImGui::MenuItem("Extract..."_j)) {
      auto folder = rsl::OpenFolder("Extract Node"_j, "");
      if (!folder)
        return;
      editor->ExtractNodeTo(node, *folder);
    }
    if (ImGui::MenuItem("Replace..."_j)) {
      std::vector<std::string> filters = {"All Files", "*"};
      if (node.is_folder()) {
        auto folder = rsl::OpenFolder("Replace With"_j, "");
        if (!folder)
          return;
        editor->ReplaceNodeWith(node, *folder);
      } else {
        std::string extension = node.name.substr(node.name.find(".") + 1);
        if (s_extension_map.contains(extension)) {
          auto& info = s_extension_map[extension];
          filters.push_back(info.m_category);
          filters.push_back(std::format("*.{}", extension));
        }
        auto file = rsl::OpenOneFile("Replace With"_j, "", filters);
        if (!file)
          return;
        editor->ReplaceNodeWith(node, *file);
      }
    }
  }
}

void RarcEditorPropertyGrid::DrawNameModal(RarcEditor* editor) {
  std::string dialog_name =
      m_original_name != "" ? "Rename Node"_j : "New Folder"_j;

  if (m_context_modal_state == ModalState::M_OPENING) {
    ImGui::OpenPopup(dialog_name.c_str());
  }

  auto flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove |
               ImGuiWindowFlags_AlwaysAutoResize;
  bool open = true;

  if (ImGui::BeginPopupModal(dialog_name.c_str(), &open, flags)) {
    RSL_DEFER(ImGui::EndPopup());

    m_context_modal_state = ModalState::M_OPEN;

    char input_buf[128]{};
    snprintf(input_buf, sizeof(input_buf), "%s", m_name_input.c_str());

    if (ImGui::InputText("Name", input_buf, sizeof(input_buf),
                         ImGuiInputTextFlags_CharsNoBlank |
                             ImGuiInputTextFlags_AutoSelectAll)) {
      m_name_input = input_buf;
    }

    bool is_invalid = m_name_input == "" || m_name_input == m_original_name;

    if (is_invalid) {
      DrawItemBorder({1.0, 0.0, 0.0, 1.0}, {{0, 0, -36, 0}});
    }

    if (!open) {
      ImGui::CloseCurrentPopup();
      m_context_modal_state = ModalState::M_CLOSED;
      return;
    }

    auto* enter_key = ImGui::GetKeyData(ImGuiKey_Enter);
    if (enter_key->Down && !is_invalid) {
      ImGui::CloseCurrentPopup();
      m_context_modal_state = ModalState::M_CLOSED;
      if (m_original_name == "")
        editor->CreateFolder(m_name_input);
      else
        editor->RenameNode(*m_focused_node, m_name_input);
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

void RarcEditorPropertyGrid::DrawSizeColumn(ResourceArchive::Node& node,
                                            RarcEditor* editor) {
  ImGui::PushID(std::addressof(node));
  RSL_DEFER(ImGui::PopID());

  std::string size_string;
  std::size_t node_size = editor->GetNodeSize(node);
  if (node_size == 0) {
    ImGui::Text("%s", "0 B");
    return;
  }

  if (node.is_folder()) {
    size_string = std::format("{}", node_size);
  } else {
    std::vector<std::string> suffixes = {"B", "KB", "MB", "GB", "TB", "PB"};

    auto size_tier = static_cast<int>(std::log10(node_size)) / 3;
    auto suffix =
        size_tier < suffixes.size() ? suffixes[size_tier] : suffixes.back();

    if (size_tier == 0) {
      size_string = std::format("{} {}", node_size, suffix);
    } else {
      size_string = std::format(
          "{} {}",
          static_cast<std::size_t>(node_size / std::pow(1000, size_tier)),
          suffix);
    }
  }

  ImGui::Text("%s", size_string.c_str());
}

void RarcEditorPropertyGrid::DrawOperationModal(RarcEditor* editor) {
  if (m_operation_state == TriState::ST_INDETERMINATE)
    return;

  std::string dialog_name =
      m_operation_state == TriState::ST_FALSE ? "Failure" : "Success";

  if (m_operation_modal_state == ModalState::M_OPENING) {
    ImGui::OpenPopup(dialog_name.c_str());
  }

  auto flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove |
               ImGuiWindowFlags_AlwaysAutoResize;
  bool open = true;

  if (ImGui::BeginPopupModal(dialog_name.c_str(), &open, flags)) {
    RSL_DEFER(ImGui::EndPopup());

    m_operation_modal_state = ModalState::M_OPEN;

    if (m_operation_state == TriState::ST_FALSE) {
      ImGui::Text("Operation failed! %s", m_operation_error_msg.c_str());
    } else {
      ImGui::Text("Operation completed successfully!");
    }

    if (!open) {
      ImGui::CloseCurrentPopup();
      m_operation_state = TriState::ST_INDETERMINATE;
      m_operation_modal_state = ModalState::M_CLOSED;
      m_operation_error_msg = "";
      return;
    }

    auto* enter_key = ImGui::GetKeyData(ImGuiKey_Enter);
    if (enter_key->Down) {
      ImGui::CloseCurrentPopup();
      m_operation_state = TriState::ST_INDETERMINATE;
      m_operation_modal_state = ModalState::M_CLOSED;
      m_operation_error_msg = "";
    }
  }
}

static void renameNode(ResourceArchive& rarc,
                       std::optional<ResourceArchive::Node>& node,
                       const std::string& new_name) {
  RSL_DEFER(node = std::nullopt);

  if (!node)
    return;

  if (new_name == "")
    return;

  auto node_it = std::find(rarc.nodes.begin(), rarc.nodes.end(), *node);
  if (node_it == rarc.nodes.end())
    return;

  node_it->name = new_name;
}

Result<void> RarcEditor::reconstruct() {
  std::optional<ResourceArchive::Node> parent_node;
  for (std::size_t i = 0; i < m_rarc.nodes.size(); ++i) {
    auto& node = m_rarc.nodes[i];
    if (node.is_folder() && node.id == m_insert_parent) {
      parent_node = node;
      break;
    }
  }

  bool changes_applied = false;

  // We order the operations in favor of search performance.
  // Realistically only one of these will be run at a time
  // So we can just recalculate the IDs once at the end.
  if (m_nodes_to_delete.size() > 0) {
    changes_applied |= librii::RARC::DeleteNodes(m_rarc, m_nodes_to_delete);
    m_nodes_to_delete.clear();
  }

  // Rename doesn't modify the node hierarchy at all.
  // So we don't need to bother flagging any changes for it.
  renameNode(m_rarc, m_node_to_rename, m_node_new_name);

  if (parent_node) {
    if (m_folder_to_create) {
      TRY(librii::RARC::CreateFolder(m_rarc, *parent_node,
                                     *m_folder_to_create));
      changes_applied = true;
    }
    if (m_files_to_insert.size() > 0) {
      auto err = TRY(
          librii::RARC::ImportFiles(m_rarc, *parent_node, m_files_to_insert));
      if (err) {
        FlagErrorDialog(err);
      }
      changes_applied = true;
    }
    if (m_folder_to_insert) {
      auto err = TRY(librii::RARC::ImportFolder(m_rarc, *parent_node,
                                                *m_folder_to_insert));
      if (err) {
        FlagErrorDialog(err);
      }
      changes_applied = true;
    }
  }

  // Extract doesn't modify the node hierarchy at all.
  // So we don't need to bother flagging any changes for it.
  if (m_node_to_extract) {
    auto err = TRY(librii::RARC::ExtractNodeTo(m_rarc, *m_node_to_extract,
                                               m_extract_path));
    if (err) {
      FlagErrorDialog(err);
    } else {
      FlagSuccessDialog();
    }
  }

  if (m_node_to_replace) {
    auto err = TRY(
        librii::RARC::ReplaceNode(m_rarc, *m_node_to_replace, m_replace_path));
    if (err) {
      FlagErrorDialog(err);
    }
    changes_applied = true;
  }

  if (changes_applied) {
    TRY(librii::RARC::RecalculateArchiveIDs(m_rarc));
    m_changes_made = true;
  }

  return {};
}

} // namespace riistudio::frontend
