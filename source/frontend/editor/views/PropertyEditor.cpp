#include "PropertyEditor.hpp"
#include <core/3d/Material.hpp>           // lib3d::Material
#include <imcxx/Widgets.hpp>              // imcxx::Combo
#include <imgui/imgui.h>                  // ImGui::Text
#include <llvm/ADT/SmallVector.h>         // llvm::SmallVector
#include <vendor/fa5/IconsFontAwesome5.h> // ICON_FA_EXCLAMATION_TRIANGLE

namespace riistudio::frontend {

class PropertyEditor : public StudioWindow {
public:
  PropertyEditor(kpi::History& host, kpi::INode& root, kpi::IObject*& active,
                 EditorWindow& ed);
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
  Mode mMode = Mode::Tabs;
  int mActiveTab = 0;
  llvm::SmallVector<bool, 16> tab_filter;

  EditorWindow& ed;
  kpi::History& mHost;
  kpi::INode& mRoot;
  kpi::IObject*& mActive;

  kpi::PropertyViewStateHolder state_holder;
};

template <typename T>
static void gatherSelected(std::set<kpi::IObject*>& tmp,
                           kpi::ICollection& folder, T pred) {
  for (int i = 0; i < folder.size(); ++i) {
    auto* obj = folder.atObject(i);
    if (folder.isSelected(i) && pred(obj)) {
      tmp.emplace(obj);
    }

    auto* col = dynamic_cast<kpi::INode*>(obj);
    if (col != nullptr) {
      for (int j = 0; j < col->numFolders(); ++j) {
        gatherSelected(tmp, *col->folderAt(i), pred);
      }
    }
  }
}

PropertyEditor::PropertyEditor(kpi::History& host, kpi::INode& root,
                               kpi::IObject*& active, EditorWindow& ed)
    : StudioWindow("Property Editor"), ed(ed), mHost(host), mRoot(root),
      mActive(active) {
  setWindowFlag(ImGuiWindowFlags_MenuBar);
  setClosable(false);
}

PropertyEditor::~PropertyEditor() { state_holder.garbageCollect(); }

static void DrawMaterialHeader(riistudio::lib3d::Material* mat) {
  if (mat->isShaderError) {
    ImGui::SetWindowFontScale(2.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{1.0f, 0.0f, 0.0f, 1.0f});
    {
      ImGui::TextUnformatted("[WARNING] Invalid shader!");
      ImGui::TextUnformatted(mat->shaderError.c_str());
    }
    ImGui::PopStyleColor();
    ImGui::SetWindowFontScale(1.0f);
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
  if (ImGui::BeginTabBar("Pane"_j)) {
    int i = 0;
    std::string title;
    manager.forEachView(
        [&](kpi::IPropertyView& view) {
          // const bool sel = mActiveTab == i;

          title.clear();
          title += view.getIcon();
          title += " ";
          title += view.getName();

          if (ImGui::BeginTabItem(title.c_str())) {
            mActiveTab = i;
            activeTab = &view;
            ImGui::EndTabItem();
          }

          ++i;
        },
        *mActive);
    ImGui::EndTabBar();
  }

  if (activeTab == nullptr) {
    mActiveTab = 0;

    ImGui::Text("Invalid Pane"_j);
    return;
  }

  activeTab->draw(*mActive, selected, mHost, mRoot, state_holder, &ed);
}

void PropertyEditor::DrawVertTabs(kpi::PropertyViewManager& manager,
                                  kpi::IPropertyView* activeTab,
                                  std::vector<kpi::IObject*>& selected) {
  ImGui::BeginChild("Left", ImVec2(120, 0), true);
  int i = 0;
  std::string title;
  manager.forEachView(
      [&](kpi::IPropertyView& view) {
        const bool sel = mActiveTab == i;

        title.clear();
        title += view.getIcon();
        title += " ";
        title += view.getName();

        if (ImGui::Selectable(title.c_str(), sel) || sel) {
          mActiveTab = i;
          activeTab = &view;
        }

        ++i;
      },
      *mActive);
  ImGui::EndChild();

  ImGui::SameLine();

  ImGui::BeginChild("Right", ImGui::GetContentRegionAvail(), true);
  {
    if (activeTab == nullptr) {
      mActiveTab = 0;

      ImGui::Text("Invalid Pane"_j);
    } else {
      activeTab->draw(*mActive, selected, mHost, mRoot, state_holder, &ed);
    }
  }
  ImGui::EndChild();
}

void PropertyEditor::DrawHeaderTabs(kpi::PropertyViewManager& manager,
                                    kpi::IPropertyView* activeTab,
                                    std::vector<kpi::IObject*>& selected) {
  std::string title;
  int i = 0;
  manager.forEachView([&](kpi::IPropertyView& view) { ++i; }, *mActive);
  const int num_headers = i;

  if (tab_filter.size() != num_headers) {
    tab_filter.resize(num_headers);
    std::fill(tab_filter.begin(), tab_filter.end(), true);
  }
  ImGui::BeginTable("Checkboxes Widget", num_headers / 5 + 1);
  ImGui::TableNextRow();
  i = 0;
  manager.forEachView(
      [&](kpi::IPropertyView& view) {
        ImGui::TableSetColumnIndex(i / 5);

        title.clear();
        title += view.getIcon();
        title += " ";
        title += view.getName();
        // TODO: >
        bool tmp = tab_filter[i];
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});
        ImGui::Checkbox(title.c_str(), &tmp);
        ImGui::PopStyleVar();
        tab_filter[i] = tmp;

        ++i;
      },
      *mActive);
  ImGui::EndTable();
  i = 0;
  manager.forEachView(
      [&](kpi::IPropertyView& view) {
        if (!tab_filter[i]) {
          ++i;
          return;
        }

        title.clear();
        title += view.getIcon();
        title += " ";
        title += view.getName();
        if (ImGui::CollapsingHeader((title + "##_TAB").c_str(),
                                    tab_filter.data() + i,
                                    ImGuiTreeNodeFlags_DefaultOpen)) {
          view.draw(*mActive, selected, mHost, mRoot, state_holder, &ed);
        }
        ++i;
      },
      *mActive);
}

void PropertyEditor::draw_() {
  auto& manager = kpi::PropertyViewManager::getInstance();

  if (mActive == nullptr) {
    ImGui::Text("Nothing is selected."_j);
    return;
  }

  if (lib3d::Material* mat = dynamic_cast<lib3d::Material*>(mActive);
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
    gatherSelected(_selected, *mRoot.folderAt(i), [&](kpi::IObject* node) {
      return node->collectionOf == mActive->collectionOf;
    });
  }
  // TODO: 7 objects per selection..?

  kpi::IPropertyView* activeTab = nullptr;

  if (_selected.empty()) {
    ImGui::Text((const char*)ICON_FA_EXCLAMATION_TRIANGLE
                " Active selection and multiselection desynced."
                " This shouldn't happen.");
    _selected.emplace(mActive);
  }
  ImGui::Text("%s %s (%i)", mActive->getName().c_str(),
              _selected.size() > 1 ? "..." : "",
              static_cast<int>(_selected.size()));

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
                                                 kpi::IObject*& active,
                                                 EditorWindow& ed) {
  return std::make_unique<PropertyEditor>(host, root, active, ed);
}

} // namespace riistudio::frontend
