#include "PropertyEditor.hpp"
#include <core/3d/Material.hpp> // lib3d::Material
#include <core/kpi/RichNameManager.hpp>
#include <core/util/gui.hpp>              // PushErrorStyle
#include <imcxx/Widgets.hpp>              // imcxx::Combo
#include <imgui/imgui.h>                  // ImGui::Text
#include <vendor/fa5/IconsFontAwesome5.h> // ICON_FA_EXCLAMATION_TRIANGLE

namespace riistudio::frontend {

class PropertyEditor : public StudioWindow {
public:
  PropertyEditor(kpi::History& host, kpi::INode& root,
                 SelectionManager& selection, EditorWindow& ed);
  ~PropertyEditor();

private:
  void DrawTabWidget(bool compact);
  void DrawHorizTabs(kpi::PropertyViewManager& manager,
                     kpi::IPropertyView* activeTab,
                     std::vector<kpi::IObject*>& selected);
  void DrawVertTabs(kpi::PropertyViewManager& manager,
                    kpi::IPropertyView* activeTab,
                    std::vector<kpi::IObject*>& selected);
  void DrawHeaderTabs(kpi::PropertyViewManager& manager,
                      kpi::IPropertyView* activeTab,
                      std::vector<kpi::IObject*>& selected);
  void draw_() override;

  enum class Mode {
    Tabs,     //!< Standard horizontal tabs
    VertTabs, //!< Tree of tabs on the left
    Headers   //!< Use collapsing headers
  };
  Mode mMode = Mode::VertTabs;
  int mActiveTab = 0;
  std::vector<bool> tab_filter;

  EditorWindow& ed;
  kpi::History& mHost;
  kpi::INode& mRoot;
  kpi::SelectionManager& mSelection;

  kpi::PropertyViewStateHolder state_holder;
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
        gatherSelected(ed, tmp, *col->folderAt(i), pred);
      }
    }
  }
}

PropertyEditor::PropertyEditor(kpi::History& host, kpi::INode& root,
                               kpi::SelectionManager& selection,
                               EditorWindow& ed)
    : StudioWindow("Property Editor"), ed(ed), mHost(host), mRoot(root),
      mSelection(selection) {
  setWindowFlag(ImGuiWindowFlags_MenuBar);
  setClosable(false);
}

PropertyEditor::~PropertyEditor() { state_holder.garbageCollect(); }

static void DrawMaterialHeader(riistudio::lib3d::Material* mat) {
  if (mat->isShaderError) {
    util::PushErrorSyle();
    {
      ImGui::TextUnformatted("[WARNING] Invalid shader!"_j);
      ImGui::TextUnformatted(mat->shaderError.c_str());
    }
    util::PopErrorStyle();
  }
}

void PropertyEditor::DrawTabWidget(bool compact) {
  if (compact)
    ImGui::PushItemWidth(150);

  mMode = imcxx::Combo(compact ? "##Property Mode" : "Property Mode"_j, mMode,
                       "Tabs\0"
                       "Vertical Tabs\0"
                       "Headers\0"_j);
  if (compact)
    ImGui::PopItemWidth();
}

void PropertyEditor::DrawHorizTabs(kpi::PropertyViewManager& manager,
                                   kpi::IPropertyView* activeTab,
                                   std::vector<kpi::IObject*>& selected) {
  // Active as an input is not strictly needed for horiztabs because imgui
  // caches that
  imcxx::TabBar bar;
  bar.active = mActiveTab;
  manager.forEachView(
      [&](kpi::IPropertyView& view) {
        std::string title = std::string(view.getIcon()) + std::string(" ") +
                            std::string(view.getName());
        bar.tabs.push_back(title);
      },
      *mSelection.mActive);

  imcxx::DrawHorizTabBar(bar);
  mActiveTab = bar.active;
  activeTab = manager.getView(bar.active, *mSelection.mActive);

  if (activeTab == nullptr) {
    mActiveTab = 0;

    ImGui::TextUnformatted("Invalid Pane"_j);
    return;
  }

  activeTab->draw(*mSelection.mActive, selected, mHost, mRoot, state_holder,
                  &ed);
}

void PropertyEditor::DrawVertTabs(kpi::PropertyViewManager& manager,
                                  kpi::IPropertyView* activeTab,
                                  std::vector<kpi::IObject*>& selected) {
  ImGui::BeginChild("Left", ImVec2(120, 0), true);
  imcxx::TabBar bar;
  bar.active = mActiveTab;
  manager.forEachView(
      [&](kpi::IPropertyView& view) {
        std::string title = std::string(view.getIcon()) + std::string(" ") +
                            std::string(view.getName());
        bar.tabs.push_back(title);
      },
      *mSelection.mActive);

  imcxx::DrawVertTabBar(bar);
  mActiveTab = bar.active;
  activeTab = manager.getView(bar.active, *mSelection.mActive);

  ImGui::EndChild();

  ImGui::SameLine();

  ImGui::BeginChild("Right", ImGui::GetContentRegionAvail(), true);
  {
    if (activeTab == nullptr) {
      mActiveTab = 0;

      ImGui::TextUnformatted("Invalid Pane"_j);
    } else {
      activeTab->draw(*mSelection.mActive, selected, mHost, mRoot, state_holder,
                      &ed);
    }
  }
  ImGui::EndChild();
}

void PropertyEditor::DrawHeaderTabs(kpi::PropertyViewManager& manager,
                                    kpi::IPropertyView* activeTab,
                                    std::vector<kpi::IObject*>& selected) {
  imcxx::CheckBoxTabBar bar;
  bar.active = tab_filter;
  manager.forEachView(
      [&](kpi::IPropertyView& view) {
        std::string title = std::string(view.getIcon()) + std::string(" ") +
                            std::string(view.getName());
        bar.tabs.push_back(title);
      },
      *mSelection.mActive);

  imcxx::DrawCheckBoxTabBar(bar);
  tab_filter = bar.active;

  std::string title;

  int i = 0;
  manager.forEachView(
      [&](kpi::IPropertyView& view) {
        if (!tab_filter[i]) {
          ++i;
          return;
        }

        title = bar.tabs[i];
        bool tmp = tab_filter[i];
        if (ImGui::CollapsingHeader((title + "##_TAB").c_str(), &tmp,
                                    ImGuiTreeNodeFlags_DefaultOpen)) {
          view.draw(*mSelection.mActive, selected, mHost, mRoot, state_holder,
                    &ed);
        }
        tab_filter[i] = tmp;
        ++i;
      },
      *mSelection.mActive);
}

void PropertyEditor::draw_() {
  auto& manager = kpi::PropertyViewManager::getInstance();

  if (mSelection.mActive == nullptr) {
    ImGui::TextUnformatted("Nothing is selected."_j);
    return;
  }

  if (lib3d::Material* mat = dynamic_cast<lib3d::Material*>(mSelection.mActive);
      mat != nullptr) {
    DrawMaterialHeader(mat);
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

  kpi::IPropertyView* activeTab = nullptr;

  if (_selected.empty()) {
    ImGui::Text((const char*)ICON_FA_EXCLAMATION_TRIANGLE
                " Active selection and multiselection desynced."
                " This shouldn't happen.");
    _selected.emplace(mSelection.mActive);
  }
  if (auto rich =
          kpi::RichNameManager::getInstance().getRich(mSelection.mActive);
      rich.hasEntry()) {
    ImGui::TextColored(
        rich.getIconColor(), "%s",
        (_selected.size() > 1 ? rich.getIconPlural() : rich.getIconSingular())
            .c_str());
    ImGui::SameLine();
    ImGui::TextUnformatted((" " +
                            (_selected.size() > 1 ? rich.getNamePlural()
                                                  : rich.getNameSingular()) +
                            ": ")
                               .c_str());
    ImGui::SameLine();
  }
  ImGui::Text("%s %s (%i)", mSelection.mActive->getName().c_str(),
              _selected.size() > 1 ? "..." : "",
              static_cast<int>(_selected.size()));
  ImGui::Separator();

  std::vector<kpi::IObject*> selected(_selected.size());
  selected.resize(0);
  std::copy(_selected.begin(), _selected.end(), std::back_inserter(selected));

  if (mMode == Mode::Tabs) {
    DrawHorizTabs(manager, activeTab, selected);
  } else if (mMode == Mode::VertTabs) {
    DrawVertTabs(manager, activeTab, selected);
  } else if (mMode == Mode::Headers) {
    DrawHeaderTabs(manager, activeTab, selected);
  } else {
    ImGui::Text("Unknown mode");
  }

  state_holder.garbageCollect();
}

std::unique_ptr<StudioWindow> MakePropertyEditor(kpi::History& host,
                                                 kpi::INode& root,
                                                 SelectionManager& selection,
                                                 EditorWindow& ed) {
  return std::make_unique<PropertyEditor>(host, root, selection, ed);
}

} // namespace riistudio::frontend
