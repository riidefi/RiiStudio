#include "PropertyEditor.hpp"
#include <core/3d/Material.hpp> // lib3d::Material
#include <core/kpi/RichNameManager.hpp>
#include <frontend/widgets/PropertyEditorWidget.hpp>
#include <imcxx/Widgets.hpp>              // imcxx::Combo
#include <vendor/fa5/IconsFontAwesome5.h> // ICON_FA_EXCLAMATION_TRIANGLE

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
    auto& manager = kpi::PropertyViewManager::getInstance();
    std::vector<std::string> r;
    manager.forEachView(
        [&](kpi::IPropertyView& view) { r.push_back(GetTitle(view)); },
        *mSelection.mActive);
    return r;
  }
  void Tab(int index) override {
    auto& manager = kpi::PropertyViewManager::getInstance();
    auto* activeTab = manager.getView(mActiveTab, *mSelection.mActive);
    if (activeTab == nullptr) {
      mActiveTab = 0;

      ImGui::TextUnformatted("Invalid Pane"_j);
    } else {
      activeTab->draw(*mSelection.mActive, selected, mHost, mRoot, state_holder,
                      &ed);
    }
  }

  EditorWindow& ed;
  kpi::History& mHost;
  kpi::INode& mRoot;
  kpi::SelectionManager& mSelection;

  kpi::PropertyViewStateHolder state_holder;
  std::vector<kpi::IObject*> selected;
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
