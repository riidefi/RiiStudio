#include "Outliner.hpp"
#include <core/kpi/RichNameManager.hpp>     // kpi::RichNameManager
#include <frontend/editor/EditorWindow.hpp> // EditorWindow
#include <frontend/editor/StudioWindow.hpp> // StudioWindow
#include <plugins/gc/Export/Material.hpp>   // libcube::IGCMaterial
//#include <regex>                          // std::regex_search
#include <core/kpi/ActionMenu.hpp> // kpi::ActionMenuManager
#include <functional>
#include <plugins/gc/Export/Scene.hpp>
#include <vendor/fa5/IconsFontAwesome5.h> // ICON_FA_SEARCH

#include "OutlinerWidget.hpp"

bool IsAdvancedMode();

namespace riistudio::frontend {

std::string NameObject(const kpi::IObject* obj, int i) {
  std::string base = obj->getName();

  if (base == "TODO") {
    const auto rich = kpi::RichNameManager::getInstance().getRich(obj);
    if (rich.hasEntry())
      base = rich.getNameSingular() + " #" + std::to_string(i);
  }

  std::string extra;

  if (IsAdvancedMode()) {
    const libcube::IGCMaterial* mat =
        dynamic_cast<const libcube::IGCMaterial*>(obj);
    if (mat != nullptr) {
      extra = "  [#Stages=" +
              std::to_string(mat->getMaterialData().mStages.size()) +
              ",#Samplers=" +
              std::to_string(mat->getMaterialData().samplers.size()) + "]";
    }
  }

  return base + extra;
}

struct GenericCollectionOutliner : public StudioWindow {
  GenericCollectionOutliner(kpi::INode& host, SelectionManager& selection,
                            EditorWindow& ed);

  void drawFolder(Child::Folder& folder) noexcept;

  void drawRecursive(std::vector<Child::Folder> folders) noexcept;

private:
  void draw_() noexcept override;

  kpi::INode& mHost;
  TFilter mFilter;
  // This points inside EditorWindow, meh
  SelectionManager& mSelection;
  // Describes the type of mActive, useful for differentiating type-disjoint
  // multiselections.
  std::string mActiveClassId;
  EditorWindow& ed;

  std::optional<std::function<void()>> activeModal;
};

GenericCollectionOutliner::GenericCollectionOutliner(
    kpi::INode& host, SelectionManager& selection, EditorWindow& ed)
    : StudioWindow("Outliner"), mHost(host), mSelection(selection), ed(ed) {
  setClosable(false);
}

std::vector<const lib3d::Texture*> GetNodeIcons(kpi::IObject& nodeAt) {
  std::vector<const lib3d::Texture*> icons;

  lib3d::Texture* tex = dynamic_cast<lib3d::Texture*>(&nodeAt);
  libcube::IGCMaterial* mat = dynamic_cast<libcube::IGCMaterial*>(&nodeAt);
  libcube::Scene* scn =
      nodeAt.childOf && nodeAt.childOf->childOf
          ? dynamic_cast<libcube::Scene*>(nodeAt.childOf->childOf)
          : nullptr;
  if (mat != nullptr) {
    for (int s = 0; s < mat->getMaterialData().samplers.size(); ++s) {
      auto& sampl = mat->getMaterialData().samplers[s];
      const lib3d::Texture* curImg = mat->getTexture(*scn, sampl.mTexture);
      // TODO: curImg pointer is unstable within container of by-value
      icons.push_back(curImg);
    }
  }
  if (tex != nullptr) {
    icons.push_back(tex);
  }

  return icons;
}

std::vector<std::optional<Child>> GetChildren(kpi::ICollection& sampler,
                                              EditorWindow& ed);

static void AddNew(kpi::ICollection* node) { node->add(); }

void DrawCtxMenuFor(kpi::IObject* nodeAt, EditorWindow& ed) {
  if (kpi::ActionMenuManager::get().drawContextMenus(*nodeAt))
    ed.getDocument().commit(ed.getSelection(), false);
}
void DrawModalFor(kpi::IObject* nodeAt, EditorWindow& ed) {
  if (kpi::ActionMenuManager::get().drawModals(*nodeAt, &ed))
    ed.getDocument().commit(ed.getSelection(), false);
}

auto GetGChildren(kpi::INode* node, EditorWindow& ed) {
  std::vector<Child::Folder> grandchildren;

  if (node != nullptr) {
    for (int i = 0; i < node->numFolders(); ++i) {
      auto children = GetChildren(*node->folderAt(i), ed);

      std::string icon_pl = "?";
      std::string name_pl = "Unknowns";

      // If no entry, we can't determine the rich name...
      if (node->folderAt(i)->size() != 0) {
        auto rich = kpi::RichNameManager::getInstance().getRich(
            node->folderAt(i)->atObject(0));

        icon_pl = rich.getIconPlural();
        name_pl = rich.getNamePlural();
      }
      grandchildren.push_back(
          Child::Folder{.children = children,
                        .key = node->idAt(i),
                        .add_new_fn = std::bind(AddNew, node->folderAt(i)),
                        .type_icon_pl = icon_pl,
                        .type_name_pl = name_pl});
    }
  }

  return grandchildren;
}

std::vector<std::optional<Child>> GetChildren(kpi::ICollection& sampler,
                                              EditorWindow& ed) {
  std::vector<std::optional<Child>> children(sampler.size());
  for (int i = 0; i < children.size(); ++i) {
    auto* node = dynamic_cast<kpi::INode*>(sampler.atObject(i));
    auto public_name = NameObject(sampler.atObject(i), i);
    auto rich =
        kpi::RichNameManager::getInstance().getRich(sampler.atObject(i));
    bool is_rich = kpi::RichNameManager::getInstance()
                       .getRich(sampler.atObject(i))
                       .hasEntry();

    auto grandchildren = GetGChildren(node, ed);

    auto* obj = sampler.atObject(i);
    std::function<void(EditorWindow&)> ctx_draw = [=](EditorWindow& ed) {
      DrawCtxMenuFor(obj, ed);
    };

    std::function<void(EditorWindow&)> modal_draw = [=](EditorWindow& ed) {
      DrawModalFor(obj, ed);
    };

    children[i] = Child{.obj = obj,
                        // .node = node,
                        .public_name = public_name,
                        .is_rich = is_rich,
                        .type_icon = rich.getIconSingular(),
                        .type_name = rich.getNameSingular(),
                        .icons_right = GetNodeIcons(*obj),
                        .folders = grandchildren,
                        .is_container = node != nullptr,
                        .draw_context_menu_fn = ctx_draw,
                        .draw_modal_fn = modal_draw};
  }

  return children;
}

void GenericCollectionOutliner::drawFolder(Child::Folder& folder) noexcept {
  DrawFolder(folder, mFilter, ed, activeModal, mSelection.mActive,
             mActiveClassId);
}

void GenericCollectionOutliner::drawRecursive(
    std::vector<Child::Folder> folders) noexcept {}

void GenericCollectionOutliner::draw_() noexcept {
  // activeModal = nullptr;
  mFilter.Draw();
  auto& _h = (kpi::INode&)mHost;
  auto children = GetGChildren(&_h, ed);
  for (auto& f : children)
    drawFolder(f);

  if (activeModal.has_value())
    (*activeModal)();
}

std::unique_ptr<StudioWindow>
MakeOutliner(kpi::INode& host, SelectionManager& selection, EditorWindow& ed) {
  return std::make_unique<GenericCollectionOutliner>(host, selection, ed);
}

} // namespace riistudio::frontend
