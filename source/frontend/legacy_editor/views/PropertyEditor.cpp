// Should be first
#include <vendor/cista.h>

#include "PropertyEditor.hpp"
#include <frontend/widgets/PropertyEditorWidget.hpp>
#include <imcxx/Widgets.hpp>              // imcxx::Combo
#include <plugins/3d/Material.hpp>        // lib3d::Material
#include <vendor/fa5/IconsFontAwesome5.h> // ICON_FA_EXCLAMATION_TRIANGLE

#include <frontend/properties/BrresBmdPropEdit.hpp>

#include "BmdBrresOutliner.hpp"

#include <frontend/PresetHelper.hpp>
#include <rsl/Defer.hpp>

namespace riistudio::frontend {

void DrawRichSelection(kpi::IObject* active, int numSelected) {
  if (auto rich = GetRichTypeInfo(active); rich) {
    // TODO: Do we want plural icons?
    auto icon = numSelected > 1 ? rich->type_icon : rich->type_icon;
    auto name = numSelected > 1 ? rich->type_name + "s" : rich->type_name;
    ImGui::TextColored(rich->type_icon_color, "%s", icon.c_str());
    ImGui::SameLine();
    ImGui::TextUnformatted((" " + name + ": ").c_str());
    ImGui::SameLine();
  }
  ImGui::Text("%s %s (%i)", active->getName().c_str(),
              numSelected > 1 ? "..." : "", numSelected);
}

template <typename T> struct CommitHandler {
  // For MatView
  bool bCommitPosted = false;
  void postUpdate() { bCommitPosted = true; }
  void consumeUpdate(kpi::History& history, T& doc) {
    assert(bCommitPosted);
    history.commit(doc);
    bCommitPosted = false;
  }
  void handleUpdates(kpi::History& history, T& doc) {
    if (bCommitPosted && !ImGui::IsAnyMouseDown())
      consumeUpdate(history, doc);
  }
  kpi::History& mHost;
  T& mRoot;
};

template <typename T> class PropertyEditor : public StudioWindow {
public:
  PropertyEditor(kpi::History& host, T& root,
                 std::function<std::set<kpi::IObject*>()> selection,
                 std::function<kpi::IObject*()> selectionActive,
                 auto drawImageIcon)
      : StudioWindow("Property Editor"), drawImageIcon(drawImageIcon),
        mHost(host), mRoot(root), mSelection(selection),
        mSelectionActive(selectionActive) {
    setWindowFlag(ImGuiWindowFlags_MenuBar);
    setClosable(false);
  }

private:
  void draw_() override;

  std::vector<std::string> TabTitles() {
    return brresBmdPropEdit.TabTitles(mSelectionActive());
  }
  bool Tab(int index) {
    auto postUpdate = [&]() { mHandler.bCommitPosted = true; };
    auto commit = [&](const char*) { mHost.commit(mRoot); };
    auto handleUpdates = [&]() { mHandler.handleUpdates(mHost, mRoot); };
    auto drawIcon = [&](const lib3d::Texture* tex, u32 dim) {
      if (tex != nullptr) {
        ImGui::SameLine();
        drawImageIcon(tex, 32);
      }
    };

    return brresBmdPropEdit.Tab(index, postUpdate, commit, handleUpdates,
                                mSelectionActive(), selected, drawIcon, mMdl);
  }
  void DrawTitle(int index) {
    brresBmdPropEdit.TabTitleFancy(mSelectionActive(), index);
  }

  std::function<void(const lib3d::Texture*, u32)> drawImageIcon;
  kpi::History& mHost;
  T& mRoot;
  std::function<std::set<kpi::IObject*>()> mSelection;
  std::function<kpi::IObject*()> mSelectionActive;

  std::vector<kpi::IObject*> selected;

  BrresBmdPropEdit brresBmdPropEdit;

  CommitHandler<T> mHandler{false, mHost, mRoot};
  riistudio::g3d::Model* mMdl = nullptr;

  PropertyEditorState m_state;
};

static void OrDialogBox(std::string_view file, u32 line,
                        std::string_view header, auto&& result) {
  if (result)
    return;
  auto err = result.error();
  rsl::ErrorDialogFmt("[{}:{}] {} failed:\n\n{}", file, line, header, err);
}
#define TRY_OR_DIALOG_BOX(task)                                                \
  OrDialogBox(__FILE_NAME__, __LINE__, #task, task)

template <typename T> void PropertyEditor<T>::draw_() {
  if (mSelectionActive() == nullptr) {
    ImGui::TextUnformatted("Nothing is selected."_j);
    return;
  }

  if (lib3d::Material* mat = dynamic_cast<lib3d::Material*>(mSelectionActive());
      mat != nullptr) {
    if (mat->isShaderError) {
      util::PushErrorSyle();
      {
        ImGui::TextUnformatted("[WARNING] Invalid shader!"_j);
        ImGui::TextUnformatted(mat->shaderError.c_str());
      }
      util::PopErrorStyle();
    }
  }

  if (ImGui::BeginPopupContextWindow()) {
    DrawPropertyEditorWidgetV2_Header(m_state, false);
    ImGui::EndPopup();
  }
  if (ImGui::BeginMenuBar()) {
    DrawPropertyEditorWidgetV2_Header(m_state, true);
    ImGui::EndMenuBar();
  }

  std::set<kpi::IObject*> _selected = mSelection();
  if (auto* g = dynamic_cast<g3d::Collection*>(&mRoot)) {
    for (auto& x : g->getModels()) {
      mMdl = &x;
    }
  }

  // TODO: 7 objects per selection..?

  if (_selected.empty()) {
    ImGui::Text((const char*)ICON_FA_EXCLAMATION_TRIANGLE
                " Active selection and multiselection desynced."
                " This shouldn't happen.");
    _selected.emplace(mSelectionActive());
  }
  DrawRichSelection(mSelectionActive(), _selected.size());
  if (lib3d::Material* mat =
          dynamic_cast<lib3d::Material*>(mSelectionActive())) {
    auto* g = dynamic_cast<g3d::Material*>(mSelectionActive());
    auto* j = dynamic_cast<j3d::Material*>(mSelectionActive());
    ImGui::SameLine();
    if (ImGui::Button((const char*)ICON_FA_LINK " Force recompile")) {
      for (auto& o : mSelection()) {
        if (lib3d::Material* m = dynamic_cast<lib3d::Material*>(o)) {
          m->nextGenerationId();
        }
      }
    }
    ImGui::SameLine();
    if (imcxx::ColoredButton(IM_COL32(0, 255, 0, 100), "{} {}",
                             (const char*)ICON_FA_DOWNLOAD,
                             "Import Preset"_j)) {
      if (g) {
        TRY_OR_DIALOG_BOX(rs::preset_helper::tryImportRsPreset(*g));
      } else if (j) {
        TRY_OR_DIALOG_BOX(rs::preset_helper::tryImportRsPresetJ(*j));
      }
    }
    ImGui::SameLine();
    if (imcxx::ColoredButton(IM_COL32(255, 0, 0, 100), "{} {}",
                             (const char*)ICON_FA_UPLOAD, "Export Preset"_j)) {
      if (g) {
        TRY_OR_DIALOG_BOX(rs::preset_helper::tryExportRsPreset(*g));
      } else if (j) {
        TRY_OR_DIALOG_BOX(rs::preset_helper::tryExportRsPresetJ(*j));
      }
    }
  }
  ImGui::Separator();

  selected = {_selected.begin(), _selected.end()};
  auto titles = TabTitles();
  std::function<bool(int)> drawTab = [&](int index) { return Tab(index); };
  std::function<void(int)> drawTitle = [&](int index) {
    return DrawTitle(index);
  };
  DrawPropertyEditorWidgetV3_Body(m_state, drawTab, drawTitle, titles);
  mMdl = nullptr;
}

std::unique_ptr<StudioWindow>
MakePropertyEditor(kpi::History& host, kpi::INode& root,
                   std::function<std::set<kpi::IObject*>()> selection,
                   std::function<kpi::IObject*()> selectionActive,
                   std::function<void(const lib3d::Texture*, u32)> icon) {
  if (auto* g = dynamic_cast<g3d::Collection*>(&root)) {
    return std::make_unique<PropertyEditor<g3d::Collection>>(
        host, *g, selection, selectionActive, icon);
  } else if (auto* g = dynamic_cast<j3d::Collection*>(&root)) {
    return std::make_unique<PropertyEditor<j3d::Collection>>(
        host, *g, selection, selectionActive, icon);
  }
  return nullptr;
}

} // namespace riistudio::frontend
