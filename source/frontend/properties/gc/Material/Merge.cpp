#include "Merge.hpp"

#include <frontend/widgets/IconManager.hpp>
#include <rsl/FsDialog.hpp>

#include <frontend/legacy_editor/views/BmdBrresOutliner.hpp>

namespace libcube::UI {

Result<std::vector<librii::crate::CrateAnimation>> tryImportManyRsPreset() {
  const auto files =
      TRY(rsl::ReadManyFile("Import Path"_j, "",
                            {
                                ".rspreset Material/Animation presets",
                                "*.rspreset",
                            }));

  std::vector<librii::crate::CrateAnimation> presets;

  for (const auto& file : files) {
    const auto& path = file.path.string();
    if (!path.ends_with(".rspreset")) {
      continue;
    }
    auto rep = librii::crate::ReadRSPreset(file.data);
    if (!rep) {
      return std::unexpected(std::format(
          "Failed to read .rspreset at \"{}\"\n{}", path, rep.error()));
    }
    presets.push_back(*rep);
  }

  return presets;
}

void DrawComboSourceMat(MergeAction& action, int& item) {
  if (item >= std::ssize(action.source)) {
    item = 0;
  }
  char buf[128];
  if (item < 0) {
    snprintf(buf, sizeof(buf), "(None)");
  } else {
    snprintf(buf, sizeof(buf), "Preset: %s %s",
             (const char*)ICON_FA_PAINT_BRUSH,
             action.source[item].mat.name.c_str());
  }

  if (ImGui::BeginCombo("##combo", buf)) {
    for (int i = -1; i < static_cast<int>(action.source.size()); ++i) {
      bool selected = i == item;
      if (i < 0) {
        snprintf(buf, sizeof(buf), "(None)");
      } else {
        snprintf(buf, sizeof(buf), "Preset: %s %s",
                 (const char*)ICON_FA_PAINT_BRUSH,
                 action.source[i].mat.name.c_str());
      }
      if (ImGui::Selectable(buf, &selected)) {
        item = i;
      }
      if (selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }
}
void DrawComboSourceMat(MergeAction& action, std::optional<size_t>& item) {
  int sel = item.value_or(-1);
  DrawComboSourceMat(action, sel);
  if (sel < 0) {
    item = std::nullopt;
  } else if (sel <= action.source.size()) {
    item = sel;
  }
}

[[nodiscard]] Result<MergeUIResult_t> DrawMergeActionUI(MergeAction& action) {
  static size_t selected_row = 0;
  if (selected_row >= action.source_selection.size()) {
    selected_row = 0;
  }

  // Child 1: no border, enable horizontal scrollbar
  {
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
    ImGui::BeginChild("ChildL",
                      ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 400),
                      false, window_flags);
    if (ImGui::BeginTable("split", 2,
                          ImGuiTableFlags_Resizable |
                              ImGuiTableFlags_NoSavedSettings |
                              ImGuiTableFlags_PadOuterX)) {
      ImGui::TableSetupColumn("Target Material");
      ImGui::TableSetupColumn("Preset to apply");
      ImGui::TableHeadersRow();
      for (size_t i = 0; i < action.source_selection.size(); i++) {
        ImGui::TableNextRow();
        char buf[128]{};
        snprintf(buf, sizeof(buf), "%s %s", (const char*)ICON_FA_PAINT_BRUSH,
                 action.target_model->getMaterials()[i].name.c_str());
        ImGui::TableSetColumnIndex(0);
        bool selected = i == selected_row;
        // Grab arrow keys with |=
        selected |= ImGui::Selectable(buf, &selected,
                                      ImGuiSelectableFlags_SpanAllColumns);
        if (selected || ImGui::IsItemFocused()) {
          selected_row = i;
        }
        ImGui::TableSetColumnIndex(1);
        auto sel = action.source_selection[i];
        if (!sel.has_value() || *sel >= action.source.size()) {
          ImGui::TextUnformatted("(None)");
        } else {
          ImGui::TextUnformatted((const char*)ICON_FA_LONG_ARROW_ALT_LEFT);
          ImGui::SameLine();
          riistudio::g3d::Material tmp;
          auto rich = riistudio::frontend::GetRichTypeInfo(&tmp).value_or(
              riistudio::frontend::Node::RichTypeInfo{});
          ImGui::PushStyleColor(ImGuiCol_Text, rich.type_icon_color);
          ImGui::TextUnformatted((const char*)ICON_FA_PAINT_BRUSH);
          ImGui::PopStyleColor();
          ImGui::SameLine();
          ImGui::TextUnformatted(
              (const char*)action.source[*sel].mat.name.c_str());
        }
      }
      ImGui::EndTable();
    }
    ImGui::EndChild();
  }

  ImGui::SameLine();

  // Child 2: rounded border
  {
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
    ImGui::BeginChild("ChildR", ImVec2(0, 400), true, window_flags);
    RSL_DEFER(ImGui::EndChild());

    ImGui::Text("Merge Material Selection:");
    EXPECT(selected_row < action.source_selection.size());
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    DrawComboSourceMat(action, action.source_selection[selected_row]);

    auto button = ImVec2{ImGui::GetContentRegionAvail().x / 2.0f, 0};
    if (ImGui::Button("Do not merge", button)) {
      TRY(action.DontMerge(selected_row));
    }
    ImGui::SameLine();
    if (ImGui::Button("Restore selection", button)) {
      TRY(action.RestoreDefault(selected_row));
    }

    auto sel = action.source_selection[selected_row];
    if (sel.has_value()) {
      EXPECT(*sel < action.source.size());
      auto& p = action.source[*sel];

      // TODO: We shouldn't need a singleton
      auto* icons = riistudio::IconManager::get();
      EXPECT(icons != nullptr);
      int it = 0;
      for (auto& samp : p.mat.samplers) {
        if (it++ != 0) {
          ImGui::SameLine();
        }
        auto tex = std::ranges::find_if(
            p.tex, [&](auto& x) { return x.name == samp.mTexture; });
        EXPECT(tex != p.tex.end());
        // TODO: This burns through generation IDs
        riistudio::g3d::Texture dyn;
        auto path = p.mat.name + tex->name;
        auto hash = std::hash<std::string>()(path);
        static_cast<librii::g3d::TextureData&>(dyn) = *tex;
        dyn.mGenerationId = hash;
        icons->drawImageIcon(&dyn, 64);
      }

      if (ImGui::BeginTable("Info", 2, ImGuiTableFlags_PadOuterX)) {
        ImGui::TableSetupColumn("Field");
        ImGui::TableSetupColumn("Value");
        ImGui::TableHeadersRow();

        auto DrawRow = [&](const char* desc, const char* val) {
          ImGui::TableNextRow();
          ImGui::TableSetColumnIndex(0);
          ImGui::TextUnformatted(desc);
          ImGui::TableSetColumnIndex(1);
          ImGui::TextUnformatted(val);
        };
        DrawRow("Author", librii::crate::GetAuthor(p).value_or("?").c_str());
        DrawRow("Date", librii::crate::GetDateCreated(p).value_or("?").c_str());
        DrawRow("Comment", librii::crate::GetComment(p).value_or("?").c_str());
        DrawRow("Tool", librii::crate::GetTool(p).value_or("?").c_str());
        ImGui::EndTable();
      }
      ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt,
                            ImGui::GetStyleColorVec4(ImGuiCol_TableRowBg));
      RSL_DEFER(ImGui::PopStyleColor());

      if (ImGui::BeginTable("Animations", 2,
                            ImGuiTableFlags_PadOuterX |
                                ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Type");
        ImGui::TableSetupColumn("Included?");
        ImGui::TableHeadersRow();

        auto DrawRow = [&](const char* desc, size_t count) {
          bool included = count > 0;

          ImGui::TableNextRow();
          ImGui::TableSetColumnIndex(0);
          if (included) {
            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg,
                                   IM_COL32(0, 255, 0, 50));
          } else {
            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg,
                                   IM_COL32(255, 0, 0, 50));
          }

          ImGui::TextUnformatted(desc);
          ImGui::TableSetColumnIndex(1);
          if (included) {
            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg,
                                   IM_COL32(0, 255, 0, 50));
          } else {
            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg,
                                   IM_COL32(255, 0, 0, 50));
          }
          ImGui::Text("%s", included ? "Yes" : "No");
        };
        DrawRow("Texture SRT animation", p.srt.size());
        DrawRow("Texture pattern animation", p.pat.size());
        DrawRow("Shader color animation", p.clr.size());
        ImGui::EndTable();
      }
    }
    ImGui::SetCursorPosY(ImGui::GetWindowContentRegionMax().y - 48.0f);
    auto wide = ImVec2{ImGui::GetContentRegionAvail().x, 0};
    if (ImGui::Button("Merge nothing", wide)) {
      TRY(action.MergeNothing());
    }
    if (ImGui::Button("Restore defaults", wide)) {
      TRY(action.RestoreDefaults());
    }
  }
  ImGui::Separator();

  auto button = ImVec2{75, 0};
  ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x - button.x * 2);
  {
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 255, 0, 100));
    RSL_DEFER(ImGui::PopStyleColor());
    if (ImGui::Button("OK", button)) {
      auto logs = TRY(action.PerformMerge());
      return MergeUIResult_t{MergeUIResult::OK{std::move(logs)}};
    }
  }
  ImGui::SameLine();
  {
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(255, 0, 0, 100));
    RSL_DEFER(ImGui::PopStyleColor());
    if (ImGui::Button("Cancel", button)) {
      return MergeUIResult_t{MergeUIResult::Cancel{}};
    }
  }
  return MergeUIResult_t{MergeUIResult::None{}};
}

} // namespace libcube::UI
