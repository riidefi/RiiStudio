#pragma once

#include <core/util/oishii.hpp>
#include <frontend/IEditor.hpp>
#include <frontend/legacy_editor/StudioWindow.hpp>
#include <frontend/widgets/AutoHistory.hpp>
#include <frontend/widgets/PropertyEditorWidget.hpp>
#include <imcxx/IndentedTreeWidget.hpp>
#include <librii/egg/Blight.hpp>
#include <rsl/FsDialog.hpp>

namespace riistudio::frontend {

// Implements a single tab for now
class BlightEditorTabSheet : private PropertyEditorWidget {
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
  std::vector<std::string> TabTitles() override { return {"BLIGHT"}; }
  bool Tab(int index) override {
    m_drawTab();
    return true;
  }

  std::function<void(void)> m_drawTab = nullptr;
};

class BlightEditorPropertyGrid {
public:
  void DrawLobj(librii::egg::Light& x, std::span<const librii::gx::Color> ambs);
  void DrawAmb(librii::gx::Color& x);
};

class BlightEditorTreeView : private imcxx::IndentedTreeWidget {
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
  u32 num_amb = 0;
  u32 selected = 0;
  std::vector<ImVec4> colors;
  std::vector<bool> enabled;

private:
  int Indent(size_t i) const override {
    if (i == 0)
      return 0;
    if (i == num_entries + 1)
      return 0;
    return 1;
  }
  bool Enabled(size_t i) const override { return true; }
  bool DrawNode(size_t i, size_t filteredIndex, bool hasChild) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    if (i == 0) {
      auto str = std::format("Lights ({} entries)", num_entries);
      bool b = ImGui::TreeNodeEx(str.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
      ImGui::TableNextColumn();
      return b;
    }
    if (i == num_entries + 1) {
      auto str = std::format("Ambient Colors ({} entries)", num_amb);
      bool b = ImGui::TreeNodeEx(str.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
      ImGui::TableNextColumn();
      return b;
    }
    u32 x = i > num_entries ? i - 2 : i - 1;
    util::ConditionalEnabled g(x < enabled.size() && enabled[x]);
    auto str =
        std::format("Entry {}", i > num_entries ? i - 2 - num_entries : i - 1);
    if (ImGui::Selectable(str.c_str(), x == selected) ||
        ImGui::IsItemFocused()) {
      selected = x;
    }
    ImGui::TableNextColumn();
    if (x < colors.size()) {
      auto c = colors[x];
      ImGui::ColorEdit4(std::format("##C{}", x + 1).c_str(), &c.x,
                        ImGuiColorEditFlags_NoPicker |
                            ImGuiColorEditFlags_NoInputs |
                            ImGuiColorEditFlags_NoLabel);
    }
    return false;
  }
  void CloseNode() override { ImGui::TreePop(); }
  int NumNodes() const override { return 2 + num_entries + num_amb; }
};

struct LobjIdx {
  u32 id;
};
struct AmbIdx {
  u32 id;
};
using BlightSelection = std::variant<LobjIdx, AmbIdx>;

class BlightEditor : public frontend::StudioWindow, public IEditor {
public:
  BlightEditor(std::span<const u8> buf, std::string_view path)
      : StudioWindow("BLIGHT Editor: <unknown>", DockSetting::Dockspace) {
    auto blm = librii::egg::ReadBLIGHT(buf, path);
    if (!blm) {
      rsl::error(blm.error());
      rsl::ErrorDialog(blm.error());
      return;
    }
    m_blight = *blm;
    m_path = path;
    setName("BLIGHT Editor: " + std::string(m_path));
  }
  BlightEditor(const BlightEditor&) = delete;
  BlightEditor(BlightEditor&&) = delete;

  void draw_() override {
    setName("BLIGHT Editor: " + m_path);

    if (ImGui::Begin((idIfyChild("Outliner")).c_str())) {
      m_tree.num_entries = m_blight.lights.size();
      m_tree.num_amb = m_blight.ambientColors.size();
      if (auto* x = std::get_if<LobjIdx>(&m_selected)) {
        m_tree.selected = x->id;
      } else if (auto* x = std::get_if<AmbIdx>(&m_selected)) {
        m_tree.selected = m_tree.num_entries + x->id;
      }
      m_tree.colors.clear();
      m_tree.enabled.clear();
      for (auto& x : m_blight.lights) {
        librii::gx::ColorF32 f = x.color;
        m_tree.colors.push_back({f.r, f.g, f.b, f.a});
        m_tree.enabled.push_back(true);
      }
      for (auto& x : m_blight.ambientColors) {
        librii::gx::ColorF32 f = x;
        m_tree.colors.push_back({f.r, f.g, f.b, f.a});
        m_tree.enabled.push_back(true);
      }
      m_tree.Draw();
      if (m_tree.selected >= m_tree.num_entries) {
        m_selected = AmbIdx{.id = m_tree.selected - m_tree.num_entries};
      } else {
        m_selected = LobjIdx{.id = m_tree.selected};
      }
    }
    ImGui::End();
    if (ImGui::Begin((idIfyChild("Properties")).c_str())) {
      if (auto* x = std::get_if<LobjIdx>(&m_selected)) {
        if (x->id < m_blight.lights.size()) {
          m_sheet.Draw([&]() {
            m_grid.DrawLobj(m_blight.lights[x->id], m_blight.ambientColors);
          });
        }
      } else if (auto* x = std::get_if<AmbIdx>(&m_selected)) {
        if (x->id < m_blight.ambientColors.size()) {
          m_sheet.Draw(
              [&]() { m_grid.DrawAmb(m_blight.ambientColors[x->id]); });
        }
      }
    }
    ImGui::End();

    m_history.update(m_blight);
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
    return "Editing a lights file (.blight)";
  }
  void saveButton() override {
    if (m_path.empty()) {
      saveAsButton();
      return;
    }
    WriteBLIGHT(m_blight, m_path);
  }
  void saveAsButton() override {
    auto default_filename = std::filesystem::path(m_path).filename();
    std::vector<std::string> filters{"EGG Binary Light (*.blight)", "*.blight",
                                     "EGG Binary Light (*.plight)", "*.plight"};
    auto results =
        rsl::SaveOneFile("Save File"_j, default_filename.string(), filters);
    if (!results) {
      rsl::error(results.error());
      rsl::ErrorDialog("Cannot save. " + results.error());
      return;
    }
    auto path = results->string();
    if (!path.ends_with(".blight") && !path.ends_with(".plight")) {
      path += ".blight";
    }
    WriteBLIGHT(m_blight, path);
  }

private:
  BlightEditorPropertyGrid m_grid;
  BlightEditorTabSheet m_sheet;
  BlightEditorTreeView m_tree;
  std::string m_path;
  librii::egg::LightSet m_blight;
  lvl::AutoHistory<librii::egg::LightSet> m_history;
  BlightSelection m_selected{LobjIdx{0}};
};

} // namespace riistudio::frontend
