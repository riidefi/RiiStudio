#include "Outliner.hpp"
#include <LibBadUIFramework/ActionMenu.hpp>        // kpi::ActionMenuManager
#include <LibBadUIFramework/RichNameManager.hpp>   // kpi::RichNameManager
#include <frontend/legacy_editor/EditorWindow.hpp> // EditorWindow
#include <frontend/legacy_editor/StudioWindow.hpp> // StudioWindow
#include <frontend/widgets/OutlinerWidget.hpp>
#include <plugins/gc/Export/Material.hpp> // libcube::IGCMaterial
#include <plugins/gc/Export/Scene.hpp>
#include <vendor/fa5/IconsFontAwesome5.h> // ICON_FA_SEARCH

IMPORT_STD;

bool IsAdvancedMode();

namespace riistudio::frontend {
static ImVec4 Clr(u32 x) {
  return ImVec4{
      static_cast<float>(x >> 16) / 255.0f,
      static_cast<float>((x >> 8) & 0xff) / 255.0f,
      static_cast<float>(x & 0xff) / 255.0f,
      1.0f,
  };
}
static std::map<std::string, Node::RichTypeInfo> richtypes{
    {"bone", {(const char*)ICON_FA_BONE, Clr(0xFFCA3A), "Bone"}},
    {"material", {(const char*)ICON_FA_PAINT_BRUSH, Clr(0x6A4C93), "Material"}},
    {"texture", {(const char*)ICON_FA_IMAGE, Clr(0x8AC926), "Texture"}},
    {"polygon", {(const char*)ICON_FA_DRAW_POLYGON, Clr(0xFFFFFF), "Polygon"}},
    {"model", {(const char*)ICON_FA_CUBE, Clr(0x1982C4), "Model"}},
    {"scene", {(const char*)ICON_FA_SHAPES, Clr(0xFFFFFF), "Scene"}},
    // g3d
    {"vc", {(const char*)ICON_FA_BRUSH, Clr(0xFFFFFF), "Vertex Color"}},
    {"srt0",
     {(const char*)ICON_FA_WAVE_SQUARE, Clr(0xFF595E),
      "Texture Matrix Animation"}},
};

std::optional<Node::RichTypeInfo> GetRichTypeInfo(const kpi::IObject* node) {
  if (auto* x = dynamic_cast<const lib3d::Bone*>(node)) {
    return richtypes["bone"];
  }
  if (auto* x = dynamic_cast<const lib3d::Material*>(node)) {
    return richtypes["material"];
  }
  if (auto* x = dynamic_cast<const lib3d::Texture*>(node)) {
    return richtypes["texture"];
  }
  if (auto* x = dynamic_cast<const lib3d::Polygon*>(node)) {
    return richtypes["polygon"];
  }
  if (auto* x = dynamic_cast<const lib3d::Model*>(node)) {
    return richtypes["model"];
  }
  if (auto* x = dynamic_cast<const lib3d::Scene*>(node)) {
    return richtypes["scene"];
  }
  if (auto* x = dynamic_cast<const g3d::ColorBuffer*>(node)) {
    return richtypes["vc"];
  }
  if (auto* x = dynamic_cast<const g3d::SRT0*>(node)) {
    return richtypes["srt0"];
  }
  return {};
}

bool ShouldBeDefaultOpen(const Node& folder) {
  // Polygons
  if (folder.rti.type_icon == (const char*)ICON_FA_DRAW_POLYGON)
    return false;
  // Vertex Colors
  if (folder.rti.type_icon == (const char*)ICON_FA_BRUSH)
    return false;

  return true;
}
// For models and bones we disable "add new" functionality for some reason
static bool CanCreateNew(std::string_view key) {
  return !key.ends_with("Model") && !key.ends_with("Bone") &&
         !key.ends_with("Texture");
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

std::string NameObject(const kpi::IObject* obj, int i) {
  std::string base = obj->getName();

  if (base == "TODO") {
    const auto rich = GetRichTypeInfo(obj);
    if (rich)
      base = rich->type_name + " #" + std::to_string(i);
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
struct ImTFilter : public ImGuiTextFilter {
  bool test(const std::string& str) const noexcept {
    return PassFilter(str.c_str());
  }
};
using TFilter = ImTFilter;
struct GenericCollectionOutliner : public StudioWindow, private OutlinerWidget {
  GenericCollectionOutliner(kpi::INode& host, SelectionManager& selection,
                            EditorWindow& ed);
  ~GenericCollectionOutliner();

  void onUndoRedo() noexcept { activeModal = std::nullopt; }

public:
  void draw_() noexcept override;

  kpi::INode& mHost;
  TFilter mFilter;
  // This points inside EditorWindow, meh
  SelectionManager& mSelection;
  EditorWindow& ed;

  // OutlinerWidget
  bool isSelected(const Node& n) const override {
    assert(n.obj != nullptr);
    return mSelection.isSelected(n.obj);
  }
  void select(const Node& n) override {
    assert(n.obj != nullptr);
    mSelection.select(n.obj);
  }
  void deselect(const Node& n) override {
    assert(n.obj != nullptr);
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
    ed.getDocument().commit(ed.getSelection(), true);
  }
  void drawImageIcon(const riistudio::lib3d::Texture* pImg,
                     int icon_size) override {
    ed.drawImageIcon(pImg, icon_size);
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

Node HeaderBar(const char* key, int indent, auto* folder,
               std::function<void()> postDeleteChild) {
  std::function<void(size_t)> delete_child_fn =
      [node = folder, postDeleteChild = postDeleteChild](size_t i) {
        DeleteChild(node, i);
        postDeleteChild();
      };
  return Node{
      .nodeType = NODE_FOLDER,
      .indent = indent,
      .rti = richtypes[key],
      .add_new_fn = std::bind(AddNew, folder),
      .delete_child_fn = delete_child_fn,
      .key = key,
      .public_name = richtypes[key].type_name + "s",
      .is_container = true,
      .is_rich = true,
  };
}

auto CtxDraw(auto* tex) {
  std::function<void(OutlinerWidget*)> ctx_draw =
      [DrawCtxMenuFor = DrawCtxMenuFor, obj = tex](OutlinerWidget* widget) {
        auto& ed = ((GenericCollectionOutliner*)(widget))->ed;
        DrawCtxMenuFor(obj, ed);
      };
  return ctx_draw;
}
auto ModalDraw(auto* tex) {
  std::function<void(OutlinerWidget*)> modal_draw =
      [DrawModalFor = DrawModalFor, obj = tex](OutlinerWidget* widget) {
        auto& ed = ((GenericCollectionOutliner*)(widget))->ed;
        DrawModalFor(obj, ed);
      };
  return modal_draw;
}

std::vector<Node> CollectNodes(g3d::Collection* g3d,
                               std::function<void()> outliner) {
  std::vector<Node> result;

  Node n{
      .nodeType = NODE_OBJECT,
      .rti = richtypes["scene"],
      .public_name = "Scene #0",
      .obj = g3d,
      .is_rich = true,
  };
  result.push_back(n);
  result.push_back(HeaderBar("model", 1, g3d->getModels().low, outliner));
  for (int i = 0; i < g3d->getModels().size(); ++i) {
    auto& mdl = g3d->getModels()[i];
    Node n{
        .indent = 2,
        .rti = richtypes["model"],
        .can_create_new = false,
        .icons_right = GetNodeIcons(mdl),
        .draw_context_menu_fn = CtxDraw(&mdl),
        .draw_modal_fn = ModalDraw(&mdl),
        .public_name = mdl.mName,
        .obj = &mdl,
        .is_container = false,
        .is_rich = true,
        .display_id_relative_to_parent = static_cast<int>(i),
    };
    result.push_back(n);
    result.push_back(
        HeaderBar("material", 3, mdl.getMaterials().low, outliner));
    for (int i = 0; i < mdl.getMaterials().size(); ++i) {
      auto& tex = mdl.getMaterials()[i];
      Node n{
          .indent = 4,
          .rti = richtypes["material"],
          .icons_right = GetNodeIcons(tex),
          .draw_context_menu_fn = CtxDraw(&tex),
          .draw_modal_fn = ModalDraw(&tex),
          .public_name = tex.name,
          .obj = &tex,
          .is_container = false,
          .is_rich = true,
          .display_id_relative_to_parent = static_cast<int>(i),
      };
      result.push_back(n);
    }
    result.push_back(HeaderBar("bone", 3, mdl.getBones().low, outliner));
    for (int i = 0; i < mdl.getBones().size(); ++i) {
      auto& bone = mdl.getBones()[i];
      Node n{
          .indent = 4,
          .rti = richtypes["bone"],
          .can_create_new = false,
          .icons_right = GetNodeIcons(bone),
          .draw_context_menu_fn = CtxDraw(&bone),
          .draw_modal_fn = ModalDraw(&bone),
          .public_name = bone.mName,
          .obj = &bone,
          .is_container = false,
          .is_rich = true,
          .display_id_relative_to_parent = static_cast<int>(i),
      };
      auto parent = bone.getBoneParent();
      int depth = 0;
      auto* it = &bone;
      while (parent >= 0) {
        it = &mdl.getBones()[parent];
        parent = it->getBoneParent();
        ++depth;
      }
      n.indent += depth;
      result.push_back(n);
    }
    result.push_back(HeaderBar("polygon", 3, mdl.getMeshes().low, outliner));
    for (int i = 0; i < mdl.getMeshes().size(); ++i) {
      auto& tex = mdl.getMeshes()[i];
      Node n{
          .indent = 4,
          .rti = richtypes["polygon"],
          .icons_right = GetNodeIcons(tex),
          .draw_context_menu_fn = CtxDraw(&tex),
          .draw_modal_fn = ModalDraw(&tex),
          .public_name = tex.mName,
          .obj = &tex,
          .is_container = false,
          .is_rich = true,
          .display_id_relative_to_parent = static_cast<int>(i),
      };
      result.push_back(n);
    }
    result.push_back(HeaderBar("vc", 3, mdl.getBuf_Clr().low, outliner));
    for (int i = 0; i < mdl.getBuf_Clr().size(); ++i) {
      auto& tex = mdl.getBuf_Clr()[i];
      Node n{
          .indent = 4,
          .rti = richtypes["vc"],
          .icons_right = GetNodeIcons(tex),
          .draw_context_menu_fn = CtxDraw(&tex),
          .draw_modal_fn = ModalDraw(&tex),
          .public_name = tex.mName,
          .obj = &tex,
          .is_container = false,
          .is_rich = true,
          .display_id_relative_to_parent = static_cast<int>(i),
      };
      result.push_back(n);
    }
  }
  result.push_back(HeaderBar("texture", 1, g3d->getTextures().low, outliner));
  for (int i = 0; i < g3d->getTextures().size(); ++i) {
    auto& tex = g3d->getTextures()[i];
    Node n{
        .indent = 2,
        .rti = richtypes["texture"],
        .can_create_new = false,
        .icons_right = GetNodeIcons(tex),
        .draw_context_menu_fn = CtxDraw(&tex),
        .draw_modal_fn = ModalDraw(&tex),
        .public_name = tex.name,
        .obj = &tex,
        .is_container = false,
        .is_rich = true,
        .display_id_relative_to_parent = static_cast<int>(i),
    };
    result.push_back(n);
  }
  result.push_back(HeaderBar("srt0", 1, g3d->getAnim_Srts().low, outliner));
  for (int i = 0; i < g3d->getAnim_Srts().size(); ++i) {
    auto& tex = g3d->getAnim_Srts()[i];
    Node n{
        .indent = 2,
        .rti = richtypes["srt0"],
        .icons_right = GetNodeIcons(tex),
        .draw_context_menu_fn = CtxDraw(&tex),
        .draw_modal_fn = ModalDraw(&tex),
        .public_name = tex.name,
        .obj = &tex,
        .is_container = false,
        .is_rich = true,
        .display_id_relative_to_parent = static_cast<int>(i),
    };
    result.push_back(n);
  }

  return result;
}

std::vector<Node> CollectNodes(j3d::Collection* g3d,
                               std::function<void()> outliner) {
  std::vector<Node> result;

  Node n{
      .nodeType = NODE_OBJECT,
      .rti = richtypes["scene"],
      .public_name = "Scene #0",
      .obj = g3d,
      .is_rich = true,
  };
  result.push_back(n);
  result.push_back(HeaderBar("model", 1, g3d->getModels().low, outliner));
  for (int i = 0; i < g3d->getModels().size(); ++i) {
    auto& mdl = g3d->getModels()[i];
    Node n{
        .indent = 2,
        .rti = richtypes["model"],
        .can_create_new = false,
        .icons_right = GetNodeIcons(mdl),
        .draw_context_menu_fn = CtxDraw(&mdl),
        .draw_modal_fn = ModalDraw(&mdl),
        .public_name = mdl.getName(),
        .obj = &mdl,
        .is_container = false,
        .is_rich = true,
        .display_id_relative_to_parent = static_cast<int>(i),
    };
    result.push_back(n);
    result.push_back(
        HeaderBar("material", 3, mdl.getMaterials().low, outliner));
    for (int i = 0; i < mdl.getMaterials().size(); ++i) {
      auto& tex = mdl.getMaterials()[i];
      Node n{
          .indent = 4,
          .rti = richtypes["material"],
          .icons_right = GetNodeIcons(tex),
          .draw_context_menu_fn = CtxDraw(&tex),
          .draw_modal_fn = ModalDraw(&tex),
          .public_name = tex.name,
          .obj = &tex,
          .is_container = false,
          .is_rich = true,
          .display_id_relative_to_parent = static_cast<int>(i),
      };
      result.push_back(n);
    }
    result.push_back(HeaderBar("bone", 3, mdl.getBones().low, outliner));
    for (int i = 0; i < mdl.getBones().size(); ++i) {
      auto& bone = mdl.getBones()[i];
      Node n{
          .indent = 4,
          .rti = richtypes["bone"],
          .can_create_new = false,
          .icons_right = GetNodeIcons(bone),
          .draw_context_menu_fn = CtxDraw(&bone),
          .draw_modal_fn = ModalDraw(&bone),
          .public_name = bone.name,
          .obj = &bone,
          .is_container = false,
          .is_rich = true,
          .display_id_relative_to_parent = static_cast<int>(i),
      };
      auto parent = bone.getBoneParent();
      int depth = 0;
      auto* it = &bone;
      while (parent >= 0) {
        it = &mdl.getBones()[parent];
        parent = it->getBoneParent();
        ++depth;
      }
      n.indent += depth;
      result.push_back(n);
    }
    result.push_back(HeaderBar("polygon", 3, mdl.getMeshes().low, outliner));
    for (int i = 0; i < mdl.getMeshes().size(); ++i) {
      auto& tex = mdl.getMeshes()[i];
      Node n{
          .indent = 4,
          .rti = richtypes["polygon"],
          .icons_right = GetNodeIcons(tex),
          .draw_context_menu_fn = CtxDraw(&tex),
          .draw_modal_fn = ModalDraw(&tex),
          .public_name = tex.getName(),
          .obj = &tex,
          .is_container = false,
          .is_rich = true,
          .display_id_relative_to_parent = static_cast<int>(i),
      };
      result.push_back(n);
    }
  }
  result.push_back(HeaderBar("texture", 1, g3d->getTextures().low, outliner));
  for (int i = 0; i < g3d->getTextures().size(); ++i) {
    auto& tex = g3d->getTextures()[i];
    Node n{
        .indent = 2,
        .rti = richtypes["texture"],
        .can_create_new = false,
        .icons_right = GetNodeIcons(tex),
        .draw_context_menu_fn = CtxDraw(&tex),
        .draw_modal_fn = ModalDraw(&tex),
        .public_name = tex.mName,
        .obj = &tex,
        .is_container = false,
        .is_rich = true,
        .display_id_relative_to_parent = static_cast<int>(i),
    };
    result.push_back(n);
  }
  return result;
}

std::vector<Node> CollectNodes_(kpi::INode* node,
                                std::function<void()> postDeleteChild) {
  if (auto* g3d = dynamic_cast<g3d::Collection*>(node)) {
    return CollectNodes(g3d, postDeleteChild);
  }
  if (auto* j3d = dynamic_cast<j3d::Collection*>(node)) {
    return CollectNodes(j3d, postDeleteChild);
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
MakeOutliner(kpi::INode& host, SelectionManager& selection, EditorWindow& ed) {
  return std::make_unique<GenericCollectionOutliner>(host, selection, ed);
}

} // namespace riistudio::frontend
