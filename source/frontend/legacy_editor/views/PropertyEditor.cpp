// Should be first
#include <vendor/cista.h>

#include "PropertyEditor.hpp"
#include <LibBadUIFramework/RichNameManager.hpp>
#include <core/3d/Material.hpp> // lib3d::Material
#include <frontend/widgets/PropertyEditorWidget.hpp>
#include <imcxx/Widgets.hpp>              // imcxx::Combo
#include <vendor/fa5/IconsFontAwesome5.h> // ICON_FA_EXCLAMATION_TRIANGLE

#include <frontend/properties/g3d/G3dMaterialViews.hpp>

namespace riistudio::frontend {

void DrawRichSelection(kpi::IObject* active, int numSelected) {
  if (auto rich = kpi::RichNameManager::getInstance().getRich(active);
      rich.hasEntry()) {
    ImGui::TextColored(
        rich.getIconColor(), "%s",
        (numSelected > 1 ? rich.getIconPlural() : rich.getIconSingular())
            .c_str());
    ImGui::SameLine();
    ImGui::TextUnformatted(
        (" " +
         (numSelected > 1 ? rich.getNamePlural() : rich.getNameSingular()) +
         ": ")
            .c_str());
    ImGui::SameLine();
  }
  ImGui::Text("%s %s (%i)", active->getName().c_str(),
              numSelected > 1 ? "..." : "", numSelected);
}

std::string GetTitle(kpi::IPropertyView& view) {
  return std::format("{} {}", view.getIcon(), view.getName());
}

std::vector<std::string> PropertyViewManager_TabTitles(kpi::IObject& active) {
  auto& manager = kpi::PropertyViewManager::getInstance();
  std::vector<std::string> r;
  manager.forEachView(
      [&](kpi::IPropertyView& view) { r.push_back(GetTitle(view)); }, active);
  return r;
}
bool PropertyViewManager_Tab(int index, std::vector<kpi::IObject*> selected,
                             kpi::History& mHost, kpi::INode& mRoot,
                             kpi::PropertyViewStateHolder& state_holder,
                             EditorWindow& ed, kpi::IObject& active) {
  auto& manager = kpi::PropertyViewManager::getInstance();
  auto* activeTab = manager.getView(index, active);
  if (activeTab == nullptr) {
    ImGui::TextUnformatted("Invalid Pane"_j);
    return false;
  } else {
    activeTab->draw(active, selected, mHost, mRoot, state_holder, &ed);
    return true;
  }
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
                 SelectionManager& selection, EditorWindow& ed)
      : StudioWindow("Property Editor"), ed(ed), mHost(host), mRoot(root),
        mSelection(selection) {
    setWindowFlag(ImGuiWindowFlags_MenuBar);
    setClosable(false);
  }
  ~PropertyEditor() { state_holder.garbageCollect(); }

private:
  void draw_() override;

  std::vector<std::string> TabTitles() override {
    auto* g3dmdl = dynamic_cast<riistudio::g3d::Model*>(mSelection.mActive);
    if (g3dmdl != nullptr) {
      return Views_TabTitles(mG3dMdlView);
    }
    auto* j3dmdl = dynamic_cast<riistudio::j3d::Model*>(mSelection.mActive);
    if (j3dmdl != nullptr) {
      return Views_TabTitles(mJ3dMdlView);
    }
    auto* g3dmat = dynamic_cast<riistudio::g3d::Material*>(mSelection.mActive);
    if (g3dmat != nullptr) {
      return Views_TabTitles(mG3dMatView);
    }
    auto* j3dmat = dynamic_cast<riistudio::j3d::Material*>(mSelection.mActive);
    if (j3dmat != nullptr) {
      return Views_TabTitles(mJ3dMatView);
    }
    auto* g3poly = dynamic_cast<g3d::Polygon*>(mSelection.mActive);
    if (g3poly != nullptr) {
      return Views_TabTitles(mG3dPolyView);
    }
    auto* j3poly = dynamic_cast<j3d::Shape*>(mSelection.mActive);
    if (j3poly != nullptr) {
      return Views_TabTitles(mJ3dPolyView);
    }
    auto* g3bone = dynamic_cast<g3d::Bone*>(mSelection.mActive);
    if (g3bone != nullptr) {
      return Views_TabTitles(mG3dBoneView);
    }
    auto* j3bone = dynamic_cast<j3d::Joint*>(mSelection.mActive);
    if (j3bone != nullptr) {
      return Views_TabTitles(mJ3dBoneView);
    }
    auto* g3dvc =
        dynamic_cast<riistudio::g3d::ColorBuffer*>(mSelection.mActive);
    if (g3dvc != nullptr) {
      return Views_TabTitles(mG3dVcView);
    }
    auto* g3dsrt = dynamic_cast<riistudio::g3d::SRT0*>(mSelection.mActive);
    if (g3dsrt != nullptr) {
      return Views_TabTitles(mG3dSrtView);
    }
    auto* g3tex = dynamic_cast<g3d::Texture*>(mSelection.mActive);
    if (g3tex != nullptr) {
      return Views_TabTitles(mG3dTexView);
    }
    auto* j3tex = dynamic_cast<j3d::Texture*>(mSelection.mActive);
    if (j3tex != nullptr) {
      return Views_TabTitles(mG3dTexView);
    }
    return PropertyViewManager_TabTitles(*mSelection.mActive);
  }
  bool Tab(int index) override {
    auto postUpdate = [&]() { mHandler.bCommitPosted = true; };
    auto commit = [&](const char*) { mHost.commit(mRoot); };

    auto* g3dmdl = dynamic_cast<g3d::Model*>(mSelection.mActive);
    if (g3dmdl != nullptr) {
      auto dl = kpi::MakeDelegate<g3d::Model>(postUpdate, commit, g3dmdl,
                                              selected, &ed);
      auto ok = Views_Tab(mG3dMdlView, dl, index);
      mHandler.handleUpdates(mHost, mRoot);
      return ok;
    }
    auto* j3dmdl = dynamic_cast<j3d::Model*>(mSelection.mActive);
    if (j3dmdl != nullptr) {
      auto dl = kpi::MakeDelegate<j3d::Model>(postUpdate, commit, j3dmdl,
                                              selected, &ed);
      auto ok = Views_Tab(mJ3dMdlView, dl, index);
      mHandler.handleUpdates(mHost, mRoot);
      return ok;
    }
    auto* g3dmat = dynamic_cast<riistudio::g3d::Material*>(mSelection.mActive);
    if (g3dmat != nullptr) {
      auto dl = kpi::MakeDelegate<riistudio::g3d::Material>(
          postUpdate, commit, g3dmat, selected, &ed);
      auto cv = CovariantPD<g3d::Material, libcube::IGCMaterial>::from(dl);
      auto ok = Views_Tab(mG3dMatView, cv, index);
      mHandler.handleUpdates(mHost, mRoot);
      return ok;
    }
    auto* j3dmat = dynamic_cast<riistudio::j3d::Material*>(mSelection.mActive);
    if (j3dmat != nullptr) {
      auto dl = kpi::MakeDelegate<riistudio::j3d::Material>(
          postUpdate, commit, j3dmat, selected, &ed);
      auto cv = CovariantPD<j3d::Material, libcube::IGCMaterial>::from(dl);
      auto ok = Views_Tab(mJ3dMatView, cv, index);
      mHandler.handleUpdates(mHost, mRoot);
      return ok;
    }
    auto* g3poly = dynamic_cast<riistudio::g3d::Polygon*>(mSelection.mActive);
    if (g3poly != nullptr) {
      auto dl = kpi::MakeDelegate<riistudio::g3d::Polygon>(
          postUpdate, commit, g3poly, selected, &ed);
      auto cv = CovariantPD<g3d::Polygon, libcube::IndexedPolygon>::from(dl);
      auto ok = Views_Tab(mG3dPolyView, cv, index);
      mHandler.handleUpdates(mHost, mRoot);
      return ok;
    }
    auto* j3poly = dynamic_cast<riistudio::j3d::Shape*>(mSelection.mActive);
    if (j3poly != nullptr) {
      auto dl = kpi::MakeDelegate<riistudio::j3d::Shape>(postUpdate, commit,
                                                         j3poly, selected, &ed);
      auto cv = CovariantPD<j3d::Shape, libcube::IndexedPolygon>::from(dl);
      auto ok = Views_Tab(mJ3dPolyView, cv, index);
      mHandler.handleUpdates(mHost, mRoot);
      return ok;
    }
    auto* j3bone = dynamic_cast<riistudio::j3d::Joint*>(mSelection.mActive);
    if (j3bone != nullptr) {
      auto dl = kpi::MakeDelegate<riistudio::j3d::Joint>(postUpdate, commit,
                                                         j3bone, selected, &ed);
      auto cv = CovariantPD<j3d::Joint, libcube::IBoneDelegate>::from(dl);
      auto ok = Views_Tab(mJ3dBoneView, cv, index);
      mHandler.handleUpdates(mHost, mRoot);
      return ok;
    }
    auto* g3bone = dynamic_cast<riistudio::g3d::Bone*>(mSelection.mActive);
    if (g3bone != nullptr) {
      auto dl = kpi::MakeDelegate<riistudio::g3d::Bone>(postUpdate, commit,
                                                        g3bone, selected, &ed);
      auto cv = CovariantPD<g3d::Bone, libcube::IBoneDelegate>::from(dl);
      auto ok = Views_Tab(mG3dBoneView, cv, index);
      mHandler.handleUpdates(mHost, mRoot);
      return ok;
    }
    auto* g3dv = dynamic_cast<riistudio::g3d::ColorBuffer*>(mSelection.mActive);
    if (g3dv != nullptr) {
      auto dl = kpi::MakeDelegate<riistudio::g3d::ColorBuffer>(
          postUpdate, commit, g3dv, selected, &ed);
      auto ok = Views_Tab(mG3dVcView, dl, index);
      mHandler.handleUpdates(mHost, mRoot);
      return ok;
    }
    auto* g3ds = dynamic_cast<riistudio::g3d::SRT0*>(mSelection.mActive);
    if (g3ds != nullptr) {
      auto dl = kpi::MakeDelegate<riistudio::g3d::SRT0>(postUpdate, commit,
                                                        g3ds, selected, &ed);
      auto ok = Views_Tab(mG3dSrtView, dl, index);
      mHandler.handleUpdates(mHost, mRoot);
      return ok;
    }
    auto* g3tex = dynamic_cast<g3d::Texture*>(mSelection.mActive);
    if (g3tex != nullptr) {
      auto dl = kpi::MakeDelegate<g3d::Texture>(postUpdate, commit, g3tex,
                                                selected, &ed);
      auto cv = CovariantPD<g3d::Texture, libcube::Texture>::from(dl);
      auto ok = Views_Tab(mG3dTexView, cv, index);
      mHandler.handleUpdates(mHost, mRoot);
      return ok;
    }
    auto* j3tex = dynamic_cast<j3d::Texture*>(mSelection.mActive);
    if (j3tex != nullptr) {
      auto dl = kpi::MakeDelegate<j3d::Texture>(postUpdate, commit, j3tex,
                                                selected, &ed);
      auto cv = CovariantPD<j3d::Texture, libcube::Texture>::from(dl);
      auto ok = Views_Tab(mJ3dTexView, cv, index);
      mHandler.handleUpdates(mHost, mRoot);
      return ok;
    }
    return PropertyViewManager_Tab(index, selected, mHost, mRoot, state_holder,
                                   ed, *mSelection.mActive);
  }

  EditorWindow& ed;
  kpi::History& mHost;
  kpi::INode& mRoot;
  kpi::SelectionManager& mSelection;

  kpi::PropertyViewStateHolder state_holder;
  std::vector<kpi::IObject*> selected;

  riistudio::G3dMdlViews mG3dMdlView;
  riistudio::G3dMaterialViews mG3dMatView;
  riistudio::G3dPolygonViews mG3dPolyView;
  riistudio::G3dBoneViews mG3dBoneView;
  riistudio::G3dTexViews mG3dTexView;
  riistudio::G3dVcViews mG3dVcView;
  riistudio::G3dSrtViews mG3dSrtView;

  riistudio::J3dMdlViews mJ3dMdlView;
  riistudio::J3dMaterialViews mJ3dMatView;
  riistudio::J3dPolygonViews mJ3dPolyView;
  riistudio::J3dBoneViews mJ3dBoneView;
  riistudio::J3dTexViews mJ3dTexView;

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

  state_holder.garbageCollect();
}

std::unique_ptr<StudioWindow> MakePropertyEditor(kpi::History& host,
                                                 kpi::INode& root,
                                                 SelectionManager& selection,
                                                 EditorWindow& ed) {
  return std::make_unique<PropertyEditor>(host, root, selection, ed);
}

} // namespace riistudio::frontend
