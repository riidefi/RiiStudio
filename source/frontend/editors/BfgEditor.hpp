#pragma once

#include <core/util/oishii.hpp>
#include <frontend/IEditor.hpp>
#include <frontend/legacy_editor/StudioWindow.hpp>
#include <frontend/widgets/AutoHistory.hpp>
#include <frontend/widgets/PropertyEditorWidget.hpp>
#include <imcxx/IndentedTreeWidget.hpp>
#include <librii/egg/BFG.hpp>
#include <rsl/FsDialog.hpp>

namespace riistudio::frontend {

// Implements a single tab for now
class BfgEditorTabSheet : private PropertyEditorWidget {
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
  std::vector<std::string> TabTitles() override { return {"BFG"}; }
  bool Tab(int index) override {
    m_drawTab();
    return true;
  }

  std::function<void(void)> m_drawTab = nullptr;
};

class BfgEditorPropertyGrid {
public:
  void Draw(librii::egg::BFG::Entry& x);
};

class BfgEditorTreeView : private imcxx::IndentedTreeWidget {
public:
  void Draw() {
    if (ImGui::BeginTable("TABLE", 2)) {
      ImGui::TableSetupColumn("##_", ImGuiTableColumnFlags_WidthStretch, .7f);
      ImGui::TableSetupColumn("Color", ImGuiTableColumnFlags_WidthStretch, .3f);
      ImGui::TableHeadersRow();
      DrawIndentedTree();
      ImGui::EndTable();
    }
  }
  u32 num_entries = 0;
  u32 selected = 0;
  std::vector<ImVec4> colors;
  std::vector<bool> enabled;

private:
  int Indent(size_t i) const override { return i > 0 ? 1 : 0; }
  bool Enabled(size_t i) const override { return true; }
  bool DrawNode(size_t i, size_t filteredIndex, bool hasChild) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    if (i == 0) {
      auto str = std::format("BFG file ({} entries)", num_entries);
      bool b = ImGui::TreeNodeEx(str.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
      ImGui::TableNextColumn();
      return b;
    }
    util::ConditionalEnabled g(i - 1 < enabled.size() && enabled[i - 1]);
    auto str = std::format("Entry {}", i);
    if (ImGui::Selectable(str.c_str(), i == selected + 1)) {
      selected = i - 1;
    }
    ImGui::TableNextColumn();
    if (i - 1 < colors.size()) {
      auto c = colors[i - 1];
      ImGui::ColorEdit4(std::format("##C{}", i).c_str(), &c.x,
                        ImGuiColorEditFlags_NoPicker |
                            ImGuiColorEditFlags_NoInputs |
                            ImGuiColorEditFlags_NoLabel);
    }
    return false;
  }
  void CloseNode() override { ImGui::TreePop(); }
  int NumNodes() const override { return 1 + num_entries; }
};
class BfgEditor : public frontend::StudioWindow, public IEditor {
public:
  BfgEditor(std::span<const u8> buf, std::string_view path)
      : StudioWindow("BFG Editor: <unknown>", DockSetting::Dockspace) {
    auto blm = librii::egg::ReadBFG(buf, path);
    if (!blm) {
      rsl::error(blm.error());
      rsl::ErrorDialog(blm.error());
      return;
    }
    m_bfg = *blm;
    m_path = path;
    setName("BFG Editor: " + std::string(m_path));
  }
  BfgEditor(const BfgEditor&) = delete;
  BfgEditor(BfgEditor&&) = delete;

  void draw_() override {
    setName("BFG Editor: " + m_path);

    if (ImGui::Begin((idIfyChild("Outliner")).c_str())) {
      m_tree.num_entries = m_bfg.entries.size();
      m_tree.selected = m_selected;
      m_tree.colors.clear();
      m_tree.enabled.clear();
      for (auto& x : m_bfg.entries) {
        m_tree.colors.push_back(util::ColorConvertU32ToFloat4BE(x.mColor));
        m_tree.enabled.push_back(x.mEnabled);
      }
      m_tree.Draw();
      m_selected = m_tree.selected;
    }
    ImGui::End();
    if (ImGui::Begin((idIfyChild("Properties")).c_str())) {
      if (m_selected < m_bfg.entries.size()) {
        m_sheet.Draw([&]() { m_grid.Draw(m_bfg.entries[m_selected]); });
      }
    }
    ImGui::End();

    m_history.update(m_bfg);
  }
  ImGuiID buildDock(ImGuiID root_id) override {
    ImGuiID next = root_id;
    ImGuiID dock_right_id =
        ImGui::DockBuilderSplitNode(next, ImGuiDir_Left, 0.4f, nullptr, &next);
    ImGuiID dock_left_id =
        ImGui::DockBuilderSplitNode(next, ImGuiDir_Right, 1.0f, nullptr, &next);

    ImGui::DockBuilderDockWindow(idIfyChild("Outliner").c_str(), dock_right_id);
    ImGui::DockBuilderDockWindow(idIfyChild("Properties").c_str(),
                                 dock_left_id);
    return next;
  }
  std::string discordStatus() const override {
    return "Editing a fog file (.bfg)";
  }
  void saveButton() override {
    if (m_path.empty()) {
      saveAsButton();
      return;
    }
    WriteBFG(m_bfg, m_path);
  }
  void saveAsButton() override {
    auto default_filename = std::filesystem::path(m_path).filename();
    std::vector<std::string> filters{"RK Binary Fog (*.bfg)", "*.bfg"};
    auto results =
        rsl::SaveOneFile("Save File"_j, default_filename.string(), filters);
    if (!results) {
      rsl::error(results.error());
      rsl::ErrorDialog("Cannot save. " + results.error());
      return;
    }
    auto path = results->string();
    if (!path.ends_with(".bfg")) {
      path += ".bfg";
    }
    WriteBFG(m_bfg, path);
  }

private:
  BfgEditorPropertyGrid m_grid;
  BfgEditorTabSheet m_sheet;
  BfgEditorTreeView m_tree;
  std::string m_path;
  librii::egg::BFG m_bfg;
  lvl::AutoHistory<librii::egg::BFG> m_history;
  u32 m_selected = 0;
};

} // namespace riistudio::frontend
