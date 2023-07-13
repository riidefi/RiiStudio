// Should be first
#include <vendor/cista.h>

#include "PropertyEditor.hpp"
#include <LibBadUIFramework/RichNameManager.hpp>
#include <core/3d/Material.hpp> // lib3d::Material
#include <frontend/widgets/PropertyEditorWidget.hpp>
#include <imcxx/Widgets.hpp>              // imcxx::Combo
#include <vendor/fa5/IconsFontAwesome5.h> // ICON_FA_EXCLAMATION_TRIANGLE

#include <frontend/properties/BrresBmdPropEdit.hpp>

#include "BmdBrresOutliner.hpp"

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

struct CommitHandler {
  // For MatView
  bool bCommitPosted = false;
  void postUpdate() { bCommitPosted = true; }
  void consumeUpdate(kpi::History& history, kpi::INode& doc) {
    assert(bCommitPosted);
    history.commit(doc);
    bCommitPosted = false;
  }
  void handleUpdates(kpi::History& history, kpi::INode& doc) {
    if (bCommitPosted && !ImGui::IsAnyMouseDown())
      consumeUpdate(history, doc);
  }
  kpi::History& mHost;
  kpi::INode& mRoot;
};

class PropertyEditor : public StudioWindow, private PropertyEditorWidget {
public:
  PropertyEditor(kpi::History& host, kpi::INode& root,
                 SelectionManager& selection, auto drawImageIcon)
      : StudioWindow("Property Editor"), drawImageIcon(drawImageIcon),
        mHost(host), mRoot(root), mSelection(selection) {
    setWindowFlag(ImGuiWindowFlags_MenuBar);
    setClosable(false);
  }

private:
  void draw_() override;

  std::vector<std::string> TabTitles() override {
    return brresBmdPropEdit.TabTitles(mSelection.mActive);
  }
  bool Tab(int index) override {
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
                                mSelection.mActive, selected, drawIcon);
  }

  std::function<void(const lib3d::Texture*, u32)> drawImageIcon;
  kpi::History& mHost;
  kpi::INode& mRoot;
  kpi::SelectionManager& mSelection;

  std::vector<kpi::IObject*> selected;

  BrresBmdPropEdit brresBmdPropEdit;

  CommitHandler mHandler{false, mHost, mRoot};
};

template <typename T>
static void gatherSelected(kpi::SelectionManager& ed,
                           std::set<kpi::IObject*>& tmp,
                           kpi::ICollection& folder, T pred) {
  for (int i = 0; i < folder.size(); ++i) {
    auto* obj = folder.atObject(i);
    if (ed.isSelected(obj) && pred(&folder)) {
      tmp.emplace(obj);
    }

    auto* col = dynamic_cast<kpi::INode*>(obj);
    if (col != nullptr) {
      for (int j = 0; j < col->numFolders(); ++j) {
        if (col->folderAt(j) != nullptr)
          gatherSelected(ed, tmp, *col->folderAt(j), pred);
      }
    }
  }
}

void PropertyEditor::draw_() {
  if (mSelection.mActive == nullptr) {
    ImGui::TextUnformatted("Nothing is selected."_j);
    return;
  }

  if (lib3d::Material* mat = dynamic_cast<lib3d::Material*>(mSelection.mActive);
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
    DrawTabWidget(false);
    ImGui::EndPopup();
  }
  if (ImGui::BeginMenuBar()) {
    DrawTabWidget(true);
    ImGui::EndMenuBar();
  }

  std::set<kpi::IObject*> _selected;
  for (int i = 0; i < mRoot.numFolders(); ++i) {
    gatherSelected(mSelection, _selected, *mRoot.folderAt(i),
                   [&](kpi::ICollection* folder) {
                     return folder == mSelection.mActive->collectionOf;
                   });
  }
  // TODO: 7 objects per selection..?

  if (_selected.empty()) {
    ImGui::Text((const char*)ICON_FA_EXCLAMATION_TRIANGLE
                " Active selection and multiselection desynced."
                " This shouldn't happen.");
    _selected.emplace(mSelection.mActive);
  }
  DrawRichSelection(mSelection.mActive, _selected.size());
  ImGui::Separator();

  selected = {_selected.begin(), _selected.end()};
  Tabs();
}

std::unique_ptr<StudioWindow>
MakePropertyEditor(kpi::History& host, kpi::INode& root,
                   SelectionManager& selection,
                   std::function<void(const lib3d::Texture*, u32)> icon) {
  return std::make_unique<PropertyEditor>(host, root, selection, icon);
}

} // namespace riistudio::frontend
