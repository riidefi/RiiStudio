#include "Outliner.hpp"
#include "BmdBrresOutliner.hpp"
#include <LibBadUIFramework/ActionMenu.hpp>        // kpi::ActionMenuManager
#include <frontend/legacy_editor/EditorWindow.hpp> // EditorWindow
#include <frontend/legacy_editor/StudioWindow.hpp> // StudioWindow
#include <frontend/widgets/OutlinerWidget.hpp>
#include <plugins/gc/Export/Material.hpp> // libcube::IGCMaterial
#include <plugins/gc/Export/Scene.hpp>
#include <vendor/fa5/IconsFontAwesome5.h> // ICON_FA_SEARCH

IMPORT_STD;

bool IsAdvancedMode();

namespace riistudio::frontend {

void DrawCtxMenuFor(kpi::IObject* nodeAt, std::function<void(bool)> commit) {
  if (kpi::ActionMenuManager::get().drawContextMenus(*nodeAt))
    commit(false);
}
void DrawModalFor(kpi::IObject* nodeAt, std::function<void(bool)> commit) {
  auto sel = kpi::ActionMenuManager::get().drawModals(*nodeAt);
  if (sel != kpi::NO_CHANGE) {
    commit(sel == kpi::CHANGE_NEED_RESET);
  }
}

struct ImTFilter : public ImGuiTextFilter {
  bool test(const std::string& str) const noexcept {
    return PassFilter(str.c_str());
  }
};
using TFilter = ImTFilter;
struct GenericCollectionOutliner : public StudioWindow, private OutlinerWidget {
  GenericCollectionOutliner(kpi::INode& host, SelectionManager& selection,
                            auto icon, auto post, auto commit);
  ~GenericCollectionOutliner();

  void onUndoRedo() noexcept { activeModal = std::nullopt; }

public:
  void draw_() noexcept override;

  kpi::INode& mHost;
  TFilter mFilter;
  // This points inside EditorWindow, meh
  SelectionManager& mSelection;

  // OutlinerWidget
  bool isSelected(const Node& n) const override {
    if (n.obj == nullptr) {
      return false;
    }
    return mSelection.isSelected(n.obj);
  }
  void select(const Node& n) override {
    if (n.obj == nullptr) {
      return;
    }
    mSelection.select(n.obj);
  }
  void deselect(const Node& n) override {
    if (n.obj == nullptr) {
      return;
    }
    mSelection.deselect(n.obj);
  }
  void clearSelectionExcept(const Node* node = nullptr) override {
    if (node == nullptr || node->obj == nullptr) {
      mSelection.selected.clear();
      return;
    }
    std::erase_if(mSelection.selected,
                  [&](const auto& x) { return x != node->obj; });
  }
  bool isActiveSelection(const Node& n) const override {
    return n.obj == mSelection.getActive();
  }
  void setActiveSelection(const Node* n) override {
    if (n == nullptr)
      mSelection.setActive(nullptr);
    else
      mSelection.setActive(n->obj);
  }
  bool hasActiveSelection() const override {
    return mSelection.getActive() != nullptr;
  }
  void postAddNew() override {
    // Commit the changes
    mPostAddNew();
  }
  void drawImageIcon(const riistudio::lib3d::Texture* pImg,
                     int icon_size) override {
    mDrawImageIcon(pImg, icon_size);
  }
  void setActiveModal(const Node* n) {
    if (n == nullptr) {
      activeModal = std::nullopt;
    } else {
      activeModal = [draw_modal = n->draw_modal_fn, f = this]() {
        if (draw_modal) {
          draw_modal(f);
        }
      };
    }
  }

public:
  std::optional<std::function<void()>> activeModal;

  std::function<void(const riistudio::lib3d::Texture*, u32)> mDrawImageIcon;
  std::function<void()> mPostAddNew;
  std::function<void(bool)> mCommit;
};

GenericCollectionOutliner::GenericCollectionOutliner(
    kpi::INode& host, SelectionManager& selection, auto icon, auto post,
    auto commit)
    : StudioWindow("Outliner"), mHost(host), mSelection(selection),
      mDrawImageIcon(icon), mPostAddNew(post), mCommit(commit) {
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
std::vector<Node> CollectNodes_(kpi::INode* node,
                                std::function<void()> postDeleteChild) {

  auto MakeCtxDraw = [&](auto* tex) {
    std::function<void(OutlinerWidget*)> ctx_draw =
        [DrawCtxMenuFor = DrawCtxMenuFor, obj = tex](OutlinerWidget* widget) {
          auto commit = ((GenericCollectionOutliner*)(widget))->mCommit;
          DrawCtxMenuFor(obj, commit);
        };
    return ctx_draw;
  };
  auto MakeModalDraw = [&](auto* tex) {
    std::function<void(OutlinerWidget*)> modal_draw =
        [DrawModalFor = DrawModalFor, obj = tex](OutlinerWidget* widget) {
          auto commit = ((GenericCollectionOutliner*)(widget))->mCommit;
          DrawModalFor(obj, commit);
        };
    return modal_draw;
  };
  if (auto* g3d = dynamic_cast<g3d::Collection*>(node)) {
    return CollectNodes(g3d, postDeleteChild, MakeCtxDraw, MakeModalDraw);
  }
  if (auto* j3d = dynamic_cast<j3d::Collection*>(node)) {
    return CollectNodes(j3d, postDeleteChild, MakeCtxDraw, MakeModalDraw);
  }
  return {};
}

void GenericCollectionOutliner::draw_() noexcept {
  // activeModal = nullptr;
  mFilter.Draw();

  auto postDeleteChild = [x = this]() { x->activeModal = {}; };
  std::vector<Node> nodes = CollectNodes_(&mHost, postDeleteChild);
  if (!nodes.empty()) {
    Node bruh = nodes[0];
    DrawFolder(std::move(nodes), bruh);
  }

  if (activeModal.has_value())
    (*activeModal)();
}

std::unique_ptr<StudioWindow>
MakeOutliner(kpi::INode& host, SelectionManager& selection,
             std::function<void(const lib3d::Texture*, u32)> icon,
             std::function<void()> post, std::function<void(bool)> commit) {
  return std::make_unique<GenericCollectionOutliner>(host, selection, icon,
                                                     post, commit);
}

} // namespace riistudio::frontend
