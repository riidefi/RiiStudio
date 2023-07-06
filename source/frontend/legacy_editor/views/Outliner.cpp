#include "Outliner.hpp"
#include <LibBadUIFramework/RichNameManager.hpp>   // kpi::RichNameManager
#include <frontend/legacy_editor/EditorWindow.hpp> // EditorWindow
#include <frontend/legacy_editor/StudioWindow.hpp> // StudioWindow
#include <plugins/gc/Export/Material.hpp>          // libcube::IGCMaterial
// #include <regex>                          // std::regex_search
#include "OutlinerWidget.hpp"
#include <LibBadUIFramework/ActionMenu.hpp> // kpi::ActionMenuManager
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
        draw_modal(f);
      };
    }
  }

public:
  std::optional<std::function<void()>> activeModal;
};

void SetNodeFromKpiObj(Node& tmp, kpi::ICollection& sampler, EditorWindow& ed,
                       size_t i, GenericCollectionOutliner* outliner) {
  auto* node = dynamic_cast<kpi::INode*>(sampler.atObject(i));
  auto public_name = NameObject(sampler.atObject(i), i);
  auto rich = GetRichTypeInfo(sampler.atObject(i));

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
  tmp = Node{
      .nodeType = NODE_OBJECT,
      .rti =
          {
              .type_icon = rich ? rich->type_icon : "?",
              .type_icon_color = rich ? rich->type_icon_color : Clr(0xFFFFFF),
              .type_name = rich ? rich->type_name : "???",
          },
      .icons_right = GetNodeIcons(*obj),
      .draw_context_menu_fn = ctx_draw,
      .draw_modal_fn = modal_draw,
      .public_name = public_name,
      .obj = obj,
      .is_container = node != nullptr,
      .is_rich = rich.has_value(),
      .display_id_relative_to_parent = static_cast<int>(i),
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
    tmp.indent = depth;
  }
}
void SetFolderFromKpi(Node& tmp, kpi::ICollection* folder, std::string key,
                      GenericCollectionOutliner* outliner) {
  std::string icon_pl = "?";
  std::string name_pl = "Unknowns";
  ImVec4 icon_color = {1.0f, 1.0f, 1.0f, 1.0f};

  // If no entry, we can't determine the rich name...
  if (folder->size() != 0) {
    auto rich = GetRichTypeInfo(folder->atObject(0));

    if (!rich) {
      rich = Node::RichTypeInfo{};
    }

    icon_pl = rich->type_icon;
    name_pl = rich->type_name;
    icon_color = rich->type_icon_color;
  } else {
    name_pl = typeid(*folder).name();
  }
  std::function<void(size_t)> delete_child_fn =
      [node = folder, outliner = outliner](size_t i) {
        DeleteChild(node, i);
        outliner->activeModal = {};
      };
  tmp = Node{
      .nodeType = NODE_FOLDER,
      .rti =
          {
              .type_icon = icon_pl,
              .type_icon_color = icon_color,
              .type_name = name_pl,
          },
      .add_new_fn = std::bind(AddNew, folder),
      .delete_child_fn = delete_child_fn,
      .key = key,
      .can_create_new = CanCreateNew(key),
  };
  tmp.default_open = ShouldBeDefaultOpen(tmp);
}

class MyTreeFlattener {
private:
  static bool FilterFolder(const kpi::ICollection& folder) {
    // Do not show empty folders
    if (folder.size() == 0)
      return true;

    // Do not display folders without rich type info (at the first child)
    if (!GetRichTypeInfo(folder.atObject(0)))
      return false;

    return true;
  }
  static bool FilterChild(const kpi::IObject& child, TFilter& filter) {
    auto* node = dynamic_cast<const kpi::INode*>(&child);
    bool container = node != nullptr && node->numFolders() > 0;
    if (!container && !filter.test(child.getName()))
      return false;

    // We rely on rich type info to infer icons
    if (!GetRichTypeInfo(&child))
      return false;

    return true;
  }

  void FromFolder(kpi::ICollection& folder, std::string key, int depth,
                  std::function<bool(const kpi::ICollection&)> filterFolder,
                  std::function<bool(const kpi::IObject&)> filterChild,
                  bool root) {
    if (!filterFolder(folder)) {
      return;
    }
    if (!root) {
      Node tmp;
      assert(mOutliner != nullptr);
      SetFolderFromKpi(tmp, &folder, key, mOutliner);
      auto& node = mNodes.emplace_back(tmp);
      node.indent = depth;
      node.nodeType = NODE_FOLDER;
      node.__numChildren = folder.size();
      ++depth;
    }
    for (size_t i = 0; i < folder.size(); ++i) {
      auto* child = folder.atObject(i);
      assert(child != nullptr);
      FromChild(*child, i, depth, filterFolder, filterChild);
    }
  }
  void FromChild(kpi::IObject& child, size_t i, int depth,
                 std::function<bool(const kpi::ICollection&)> filterFolder,
                 std::function<bool(const kpi::IObject&)> filterChild) {
    if (!filterChild(child)) {
      return;
    }
    Node tmp;
    assert(child.collectionOf);
    assert(mEditor != nullptr);
    assert(mOutliner != nullptr);
    SetNodeFromKpiObj(tmp, *child.collectionOf, *mEditor, i, mOutliner);
    auto& node = mNodes.emplace_back(tmp);
    node.indent += depth;
    node.nodeType = NODE_OBJECT;
    if (auto* node = dynamic_cast<kpi::INode*>(&child)) {
      for (size_t i = 0; i < node->numFolders(); ++i) {
        auto* folder = node->folderAt(i);
        assert(folder != nullptr);
        FromFolder(*folder, node->idAt(i), depth + 1, filterFolder, filterChild,
                   /*root*/ false);
      }
    }
  }
  std::vector<Node>&& TakeResult() { return std::move(mNodes); }

public:
  static std::vector<Node> Flatten(kpi::ICollection& folder, TFilter& objFilter,
                                   GenericCollectionOutliner& outliner,
                                   EditorWindow& editor) {
    if (!FilterFolder(folder))
      return {};
    MyTreeFlattener instance(outliner, editor);
    instance.FromFolder(
        folder, "ROOT", 0, FilterFolder,
        [&](auto& child) { return FilterChild(child, objFilter); },
        /*root*/ true);
    return instance.TakeResult();
  }

private:
  MyTreeFlattener(GenericCollectionOutliner& outliner, EditorWindow& editor)
      : mOutliner(&outliner), mEditor(&editor) {}

  GenericCollectionOutliner* mOutliner = nullptr;
  EditorWindow* mEditor = nullptr;
  std::vector<Node> mNodes;
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

void GenericCollectionOutliner::draw_() noexcept {
  // activeModal = nullptr;
  mFilter.Draw();

  class RootFolder final : public kpi::ICollection {
  public:
    RootFolder(kpi::INode& root) : mpRoot(&root) {}
    ~RootFolder() override = default;

  private:
    std::size_t size() const override { return 1; }
    void* at(std::size_t) override { return mpRoot; }
    const void* at(std::size_t) const override { return mpRoot; }
    void resize(std::size_t) override { assert(!"Not implemented"); }
    kpi::IObject* atObject(std::size_t i) override {
      assert(i == 0);
      return mpRoot;
    }
    const kpi::IObject* atObject(std::size_t i) const override {
      assert(i == 0);
      return mpRoot;
    }
    std::string atName(std::size_t i) const override {
      assert(i == 0);
      return mpRoot->getName();
    }
    void add() override { assert(!"Not implemented"); }
    void swap(std::size_t, std::size_t) override { assert(!"Not implemented"); }

  private:
    kpi::INode* mpRoot;
  };
  RootFolder root(mHost);
  // Not great, hack
  auto* back = mHost.collectionOf;
  mHost.collectionOf = &root;
  auto nodes = MyTreeFlattener::Flatten(root, mFilter, *this, ed);
  if (!nodes.empty()) {
    Node bruh = nodes[0];
    DrawFolder(std::move(nodes), bruh);
  }
  mHost.collectionOf = back;

  if (activeModal.has_value())
    (*activeModal)();
}

std::unique_ptr<StudioWindow>
MakeOutliner(kpi::INode& host, SelectionManager& selection, EditorWindow& ed) {
  return std::make_unique<GenericCollectionOutliner>(host, selection, ed);
}

} // namespace riistudio::frontend
