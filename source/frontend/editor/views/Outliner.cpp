#include "Outliner.hpp"
#include <core/kpi/RichNameManager.hpp>     // kpi::RichNameManager
#include <frontend/editor/EditorWindow.hpp> // EditorWindow
#include <frontend/editor/StudioWindow.hpp> // StudioWindow
#include <plugins/gc/Export/Material.hpp>   // libcube::IGCMaterial
// #include <regex>                          // std::regex_search
#include "OutlinerWidget.hpp"
#include <core/kpi/ActionMenu.hpp> // kpi::ActionMenuManager
#include <plugins/gc/Export/Scene.hpp>
#include <vendor/fa5/IconsFontAwesome5.h> // ICON_FA_SEARCH

IMPORT_STD;

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

struct GenericCollectionOutliner : public StudioWindow, private OutlinerWidget {
  GenericCollectionOutliner(kpi::INode& host, SelectionManager& selection,
                            EditorWindow& ed);
  ~GenericCollectionOutliner();

  void drawFolder(Child::Folder& folder) noexcept;

  void drawRecursive(std::vector<Child::Folder> folders) noexcept;

  void onUndoRedo() noexcept { activeModal = std::nullopt; }

public:
  void draw_() noexcept override;

  kpi::INode& mHost;
  TFilter mFilter;
  // This points inside EditorWindow, meh
  SelectionManager& mSelection;
  // Describes the type of mActive, useful for differentiating type-disjoint
  // multiselections.
  std::string mActiveClassId;
  EditorWindow& ed;

  // OutlinerWidget
  bool isSelected(NodeFolder& f, size_t i) const override {
    return mSelection.isSelected(f.children[i]->obj);
  }
  void select(NodeFolder& nodes, size_t index) override {
    mSelection.select(nodes.children[index]->obj);
  }
  void deselect(NodeFolder& nodes, size_t index) override {
    mSelection.deselect(nodes.children[index]->obj);
  }
  void clearSelection() override { mSelection.selected.clear(); }
  size_t getActiveSelection(NodeFolder& nodes) const override {
    auto* obj = mSelection.getActive();

    for (int i = 0; i < nodes.children.size(); ++i)
      if (nodes.children[i]->obj == obj)
        return i;

    return ~0;
  }
  void setActiveSelection(NodeFolder& nodes, size_t index) override {
    if (index == ~0)
      mSelection.setActive(nullptr);
    else
      mSelection.setActive(nodes.children[index]->obj);
  }
  bool hasActiveSelection() const override {
    return mSelection.getActive() != nullptr;
  }
  void postAddNew() override {
    // Commit the changes
    ed.getDocument().commit(ed.getSelection(), true);
  }
  void drawImageIcon(const riistudio::lib3d::Texture* pImg,
                     int icon_size) override {
    ed.drawImageIcon(pImg, icon_size);
  }

public:
  std::optional<std::function<void()>> activeModal;
};

GenericCollectionOutliner::GenericCollectionOutliner(
    kpi::INode& host, SelectionManager& selection, EditorWindow& ed)
    : StudioWindow("Outliner"), mHost(host), mSelection(selection), ed(ed) {
  setClosable(false);
  mSelection.mUndoRedoCbs.push_back(
      {this, [outliner = this] { outliner->onUndoRedo(); }});
}
GenericCollectionOutliner::~GenericCollectionOutliner() {
  auto it = std::find_if(mSelection.mUndoRedoCbs.begin(),
                         mSelection.mUndoRedoCbs.end(),
                         [&](const auto& x) { return x.first == this; });
  if (it != mSelection.mUndoRedoCbs.end()) {
    mSelection.mUndoRedoCbs.erase(it);
  }
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

std::vector<std::optional<Child>>
GetChildren(kpi::ICollection& sampler, EditorWindow& ed,
            GenericCollectionOutliner* outliner);

static void AddNew(kpi::ICollection* node) { node->add(); }
static void DeleteChild(kpi::ICollection* node, size_t index) {
  if (node->size() < 1 || index >= node->size()) {
    return;
  }
  int end = static_cast<int>(node->size()) - 1;
  for (int i = index; i < end; ++i) {
    node->swap(i, i + 1);
  }
  node->resize(end);
}

void DrawCtxMenuFor(kpi::IObject* nodeAt, EditorWindow& ed) {
  if (kpi::ActionMenuManager::get().drawContextMenus(*nodeAt))
    ed.getDocument().commit(ed.getSelection(), false);
}
void DrawModalFor(kpi::IObject* nodeAt, EditorWindow& ed) {
  auto sel = kpi::ActionMenuManager::get().drawModals(*nodeAt, &ed);
  if (sel != kpi::NO_CHANGE) {
    ed.getDocument().commit(ed.getSelection(), sel == kpi::CHANGE_NEED_RESET);
  }
}

auto GetGChildren(kpi::INode* node, EditorWindow& ed,
                  GenericCollectionOutliner* outliner) {
  std::vector<Child::Folder> grandchildren;

  if (node != nullptr) {
    for (int i = 0; i < node->numFolders(); ++i) {
      auto children = GetChildren(*node->folderAt(i), ed, outliner);

      std::string icon_pl = "?";
      std::string name_pl = "Unknowns";
      ImVec4 icon_color = {1.0f, 1.0f, 1.0f, 1.0f};

      // If no entry, we can't determine the rich name...
      if (node->folderAt(i)->size() != 0) {
        auto rich = kpi::RichNameManager::getInstance().getRich(
            node->folderAt(i)->atObject(0));

        icon_pl = rich.getIconPlural();
        name_pl = rich.getNamePlural();
        icon_color = rich.getIconColor();
      }
      std::function<void(size_t)> delete_child_fn =
          [node = node->folderAt(i), outliner = outliner](size_t i) {
            DeleteChild(node, i);
            outliner->activeModal = {};
          };
      grandchildren.push_back(Child::Folder{
          {
              .nodeType = NODE_FOLDER,
              .type_icon = icon_pl,
              .type_icon_color = icon_color,
              .type_name = name_pl,
              .add_new_fn = std::bind(AddNew, node->folderAt(i)),
              .delete_child_fn = delete_child_fn,
              .key = node->idAt(i),
          },
          .children = children,
      });
    }
  }

  return grandchildren;
}

std::vector<std::optional<Child>>
GetChildren(kpi::ICollection& sampler, EditorWindow& ed,
            GenericCollectionOutliner* outliner) {
  std::vector<std::optional<Child>> children(sampler.size());
  for (int i = 0; i < children.size(); ++i) {
    auto* node = dynamic_cast<kpi::INode*>(sampler.atObject(i));
    auto public_name = NameObject(sampler.atObject(i), i);
    auto rich =
        kpi::RichNameManager::getInstance().getRich(sampler.atObject(i));
    bool is_rich = kpi::RichNameManager::getInstance()
                       .getRich(sampler.atObject(i))
                       .hasEntry();

    auto grandchildren = GetGChildren(node, ed, outliner);

    auto* obj = sampler.atObject(i);
    std::function<void(OutlinerWidget*)> ctx_draw =
        [DrawCtxMenuFor = DrawCtxMenuFor, obj = obj](OutlinerWidget* widget) {
          auto& ed = ((GenericCollectionOutliner*)(widget))->ed;
          DrawCtxMenuFor(obj, ed);
        };

    std::function<void(OutlinerWidget*)> modal_draw =
        [DrawModalFor = DrawModalFor, obj = obj](OutlinerWidget* widget) {
          auto& ed = ((GenericCollectionOutliner*)(widget))->ed;
          DrawModalFor(obj, ed);
        };

    children[i] = Child{
        {
            .nodeType = NODE_OBJECT,
            .type_icon = rich.getIconSingular(),
            .type_icon_color = rich.getIconColor(),
            .type_name = rich.getNameSingular(),
            .draw_context_menu_fn = ctx_draw,
            .draw_modal_fn = modal_draw,
            .obj = obj,
            .public_name = public_name,
            .is_rich = is_rich,
            .icons_right = GetNodeIcons(*obj),
            .is_container = node != nullptr,
        },
        .folders = grandchildren,
    };
    if (auto* bone = dynamic_cast<riistudio::lib3d::Bone*>(obj)) {
      auto parent = bone->getBoneParent();
      int depth = 0;
      while (parent >= 0) {
        bone = dynamic_cast<riistudio::lib3d::Bone*>(
            obj->collectionOf->atObject(parent));
        parent = bone->getBoneParent();
        ++depth;
      }
      children[i]->indent = depth;
    }
  }

  return children;
}

void GenericCollectionOutliner::drawFolder(Child::Folder& folder) noexcept {
  DrawFolder(folder, mFilter, activeModal, mActiveClassId);
}

void GenericCollectionOutliner::drawRecursive(
    std::vector<Child::Folder> folders) noexcept {}

void GenericCollectionOutliner::draw_() noexcept {
  // activeModal = nullptr;
  mFilter.Draw();
  auto& _h = (kpi::INode&)mHost;
  auto children = GetGChildren(&_h, ed, this);
  std::function<void(OutlinerWidget*)> ctx_draw = [=](OutlinerWidget* widget) {
    auto& ed = ((GenericCollectionOutliner*)(widget))->ed;
    DrawCtxMenuFor(&mHost, ed);
  };

  std::function<void(OutlinerWidget*)> modal_draw =
      [=](OutlinerWidget* widget) {
        auto& ed = ((GenericCollectionOutliner*)(widget))->ed;
        DrawModalFor(&mHost, ed);
      };
  auto root = Child{
      {
          .nodeType = NODE_OBJECT,
          .type_icon = (const char*)ICON_FA_SHAPES,
          .type_icon_color = {1.0f, 1.0f, 1.0f, 1.0f},
          .type_name = "Scene",
          .draw_context_menu_fn = ctx_draw,
          .draw_modal_fn = modal_draw,
          .obj = &mHost,
          .public_name =
              std::filesystem::path(ed.getFilePath()).filename().string(),
          .is_rich = true,
          .icons_right = GetNodeIcons(mHost),
          .is_container = true,
      },
      .folders = children,
  };
  auto root_folder = Child::Folder{
      {
          .nodeType = NODE_FOLDER,
          .type_icon = (const char*)ICON_FA_SHAPES,
          .type_icon_color = {1.0f, 1.0f, 1.0f, 1.0f},
          .type_name = "Scenes",
          .add_new_fn = nullptr,
          .delete_child_fn = nullptr,
          .key = "ROOT",
      },
      .children = {root},
  };
  drawFolder(root_folder);

  if (activeModal.has_value())
    (*activeModal)();
}

std::unique_ptr<StudioWindow>
MakeOutliner(kpi::INode& host, SelectionManager& selection, EditorWindow& ed) {
  return std::make_unique<GenericCollectionOutliner>(host, selection, ed);
}

} // namespace riistudio::frontend
