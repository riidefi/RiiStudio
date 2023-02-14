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
    auto* g3dmat = dynamic_cast<riistudio::g3d::Material*>(mSelection.mActive);
    if (g3dmat != nullptr) {
      return Views_TabTitles(mG3dMatView);
    }
    auto* j3dmat = dynamic_cast<riistudio::j3d::Material*>(mSelection.mActive);
    if (j3dmat != nullptr) {
      return Views_TabTitles(mJ3dMatView);
    }
    auto* gcpoly = dynamic_cast<libcube::IndexedPolygon*>(mSelection.mActive);
    if (gcpoly != nullptr) {
      return Views_TabTitles(mGcPolyView);
    }
    auto* gcbone = dynamic_cast<libcube::IBoneDelegate*>(mSelection.mActive);
    if (gcbone != nullptr) {
      return Views_TabTitles(mGcBoneView);
    }
    return PropertyViewManager_TabTitles(*mSelection.mActive);
  }
  bool Tab(int index) override {
    auto postUpdate = [&]() { mHandler.bCommitPosted = true; };
    auto commit = [&](const char*) { mHost.commit(mRoot); };

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
    auto* gcpoly = dynamic_cast<libcube::IndexedPolygon*>(mSelection.mActive);
    if (gcpoly != nullptr) {
      auto dl = kpi::MakeDelegate<libcube::IndexedPolygon>(
          postUpdate, commit, gcpoly, selected, &ed);
      auto ok = Views_Tab(mGcPolyView, dl, index);
      mHandler.handleUpdates(mHost, mRoot);
      return ok;
    }
    auto* gcbone = dynamic_cast<libcube::IBoneDelegate*>(mSelection.mActive);
    if (gcbone != nullptr) {
      auto dl = kpi::MakeDelegate<libcube::IBoneDelegate>(
          postUpdate, commit, gcbone, selected, &ed);
      auto ok = Views_Tab(mGcBoneView, dl, index);
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

  riistudio::G3dMaterialViews mG3dMatView;
  riistudio::J3dMaterialViews mJ3dMatView;
  riistudio::GcPolygonViews mGcPolyView;
  riistudio::GcBoneViews mGcBoneView;

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
