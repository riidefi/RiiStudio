#pragma once

#include <frontend/widgets/OutlinerWidget.hpp>
#include <imcxx/Widgets.hpp>
#include <plugins/g3d/collection.hpp>
#include <plugins/j3d/Scene.hpp>

namespace riistudio::frontend {

// TODO: Some better color scheme
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
    {"chr0",
     {(const char*)ICON_FA_WAVE_SQUARE, Clr(0x59FF5E), "Character Animation"}},
    {"clr0",
     {(const char*)ICON_FA_WAVE_SQUARE, Clr(0xA9335E),
      "Uniform Color Animation"}},
    {"pat0",
     {(const char*)ICON_FA_WAVE_SQUARE, Clr(0x33FFFF),
      "Texture Pattern Animation"}},
    {"vis0",
     {(const char*)ICON_FA_WAVE_SQUARE, Clr(0xa80077),
      "Bone Visibility Animation"}},
};

static inline std::optional<Node::RichTypeInfo>
GetRichTypeInfo(const kpi::IObject* node) {
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

static inline std::vector<const lib3d::Texture*>
GetNodeIcons(kpi::IObject& nodeAt) {
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

static inline Node HeaderBarImpl(const char* key, int indent, size_t size,
                                 std::function<void(size_t)> delete_child_fn,
                                 std::function<void()> add_new_fn,
                                 std::function<void()> postDeleteChild) {
  return Node{
      .nodeType = NODE_FOLDER,
      .indent = indent,
      .rti = richtypes[key],
      .add_new_fn = add_new_fn,
      .delete_child_fn = delete_child_fn,
      .key = key,
      .public_name = richtypes[key].type_name + "s",
      .is_container = true,
      .is_rich = true,
      .__numChildren = static_cast<int>(size),
  };
}

static inline void AddNew(kpi::ICollection* node) { node->add(); }
static inline void DeleteChild(kpi::ICollection* node, size_t index) {
  if (node->size() < 1 || index >= node->size()) {
    return;
  }
  int end = static_cast<int>(node->size()) - 1;
  for (int i = index; i < end; ++i) {
    node->swap(i, i + 1);
  }
  node->resize(end);
}

static inline Node HeaderBar(const char* key, int indent, auto* folder,
                             std::function<void()> postDeleteChild) {
  std::function<void(size_t)> delete_child_fn =
      [node = folder, postDeleteChild = postDeleteChild](size_t i) {
        DeleteChild(node, i);
        postDeleteChild();
      };
  auto add_new_fn = std::bind(AddNew, folder);
  return HeaderBarImpl(key, indent, folder->size(), delete_child_fn, add_new_fn,
                       postDeleteChild);
}

static inline std::vector<Node> CollectNodes(g3d::Collection* g3d,
                                             std::function<void()> outliner,
                                             auto CtxDraw, auto ModalDraw) {
  std::vector<Node> result;

  Node n{
      .nodeType = NODE_OBJECT,
      .rti = richtypes["scene"],
      .draw_context_menu_fn = CtxDraw(g3d),
      .draw_modal_fn = ModalDraw(g3d),
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
          .public_name = bone.getName(),
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
  auto add_new_clr0 = [pG = g3d]() {
    auto& x = pG->clrs.emplace_back();
    x.name = "Untitled CLR0";
  };
  result.push_back(HeaderBarImpl("clr0", 1, g3d->clrs.size(), nullptr,
                                 add_new_clr0, nullptr));
  for (int i = 0; i < g3d->clrs.size(); ++i) {
    auto& tex = g3d->clrs[i];
    Node n{
        .indent = 2,
        .rti = richtypes["clr0"],
        .icons_right = {},
        .draw_context_menu_fn = nullptr,
        .draw_modal_fn = nullptr,
        .public_name = tex.name,
        .obj = nullptr,
        .is_container = false,
        .is_rich = true,
        .display_id_relative_to_parent = static_cast<int>(i),
    };
    result.push_back(n);
  }
  auto add_new_pat0 = [pG = g3d]() {
    auto& x = pG->pats.emplace_back();
    x.name = "Untitled PAT0";
  };
  result.push_back(HeaderBarImpl("pat0", 1, g3d->pats.size(), nullptr,
                                 add_new_pat0, nullptr));
  for (int i = 0; i < g3d->pats.size(); ++i) {
    auto& tex = g3d->pats[i];
    Node n{
        .indent = 2,
        .rti = richtypes["pat0"],
        .icons_right = {},
        .draw_context_menu_fn = nullptr,
        .draw_modal_fn = nullptr,
        .public_name = tex.name,
        .obj = nullptr,
        .is_container = false,
        .is_rich = true,
        .display_id_relative_to_parent = static_cast<int>(i),
    };
    result.push_back(n);
  }
  auto add_new_vis0 = [pG = g3d]() {
    auto& x = pG->viss.emplace_back();
    x.name = "Untitled VIS0";
  };
  result.push_back(HeaderBarImpl("vis0", 1, g3d->viss.size(), nullptr,
                                 add_new_vis0, nullptr));
  for (int i = 0; i < g3d->viss.size(); ++i) {
    auto& tex = g3d->viss[i];
    Node n{
        .indent = 2,
        .rti = richtypes["vis0"],
        .icons_right = {},
        .draw_context_menu_fn = nullptr,
        .draw_modal_fn = nullptr,
        .public_name = tex.name,
        .obj = nullptr,
        .is_container = false,
        .is_rich = true,
        .display_id_relative_to_parent = static_cast<int>(i),
    };
    result.push_back(n);
  }
  auto add_new_chr0 = [pG = g3d]() {
    auto& x = pG->chrs.emplace_back();
    x.name = "Untitled CHR0";
  };
  result.push_back(HeaderBarImpl("chr0", 1, g3d->chrs.size(), nullptr,
                                 add_new_chr0, nullptr));
  for (int i = 0; i < g3d->chrs.size(); ++i) {
    auto& tex = g3d->chrs[i];
    Node n{
        .indent = 2,
        .rti = richtypes["chr0"],
        .icons_right = {},
        .draw_context_menu_fn = nullptr,
        .draw_modal_fn = nullptr,
        .public_name = tex.name,
        .obj = nullptr,
        .is_container = false,
        .is_rich = true,
        .display_id_relative_to_parent = static_cast<int>(i),
    };
    result.push_back(n);
  }

  return result;
}

static std::vector<Node> CollectNodes(j3d::Collection* g3d,
                                      std::function<void()> outliner,
                                      auto CtxDraw, auto ModalDraw) {
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
          .public_name = bone.getName(),
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

} // namespace riistudio::frontend
