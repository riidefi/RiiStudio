#pragma once

#include <frontend/properties/g3d/G3dMaterialViews.hpp>

namespace riistudio::frontend {

struct BrresBmdPropEdit {
  riistudio::G3dMdlViews mG3dMdlView;
  riistudio::G3dMaterialViews mG3dMatView;
  riistudio::G3dPolygonViews mG3dPolyView;
  riistudio::G3dBoneViews mG3dBoneView;
  riistudio::G3dTexViews mG3dTexView;
  riistudio::G3dVcViews mG3dVcView;
  riistudio::G3dSrtViews mG3dSrtView;

  riistudio::J3dMdlViews mJ3dMdlView;
  riistudio::J3dMaterialViews mJ3dMatView;
  riistudio::J3dPolygonViews mJ3dPolyView;
  riistudio::J3dBoneViews mJ3dBoneView;
  riistudio::J3dTexViews mJ3dTexView;

  std::vector<std::string> TabTitles(kpi::IObject* node) {
    auto* g3dmdl = dynamic_cast<riistudio::g3d::Model*>(node);
    if (g3dmdl != nullptr) {
      return Views_TabTitles(mG3dMdlView);
    }
    auto* j3dmdl = dynamic_cast<riistudio::j3d::Model*>(node);
    if (j3dmdl != nullptr) {
      return Views_TabTitles(mJ3dMdlView);
    }
    auto* g3dmat = dynamic_cast<riistudio::g3d::Material*>(node);
    if (g3dmat != nullptr) {
      return Views_TabTitles(mG3dMatView);
    }
    auto* j3dmat = dynamic_cast<riistudio::j3d::Material*>(node);
    if (j3dmat != nullptr) {
      return Views_TabTitles(mJ3dMatView);
    }
    auto* g3poly = dynamic_cast<g3d::Polygon*>(node);
    if (g3poly != nullptr) {
      return Views_TabTitles(mG3dPolyView);
    }
    auto* j3poly = dynamic_cast<j3d::Shape*>(node);
    if (j3poly != nullptr) {
      return Views_TabTitles(mJ3dPolyView);
    }
    auto* g3bone = dynamic_cast<g3d::Bone*>(node);
    if (g3bone != nullptr) {
      return Views_TabTitles(mG3dBoneView);
    }
    auto* j3bone = dynamic_cast<j3d::Joint*>(node);
    if (j3bone != nullptr) {
      return Views_TabTitles(mJ3dBoneView);
    }
    auto* g3dvc = dynamic_cast<riistudio::g3d::ColorBuffer*>(node);
    if (g3dvc != nullptr) {
      return Views_TabTitles(mG3dVcView);
    }
    auto* g3dsrt = dynamic_cast<riistudio::g3d::SRT0*>(node);
    if (g3dsrt != nullptr) {
      return Views_TabTitles(mG3dSrtView);
    }
    auto* g3tex = dynamic_cast<g3d::Texture*>(node);
    if (g3tex != nullptr) {
      return Views_TabTitles(mG3dTexView);
    }
    auto* j3tex = dynamic_cast<j3d::Texture*>(node);
    if (j3tex != nullptr) {
      return Views_TabTitles(mG3dTexView);
    }
    return {};
  }
  void TabTitleFancy(kpi::IObject* node, int index) {
    auto* g3dmdl = dynamic_cast<riistudio::g3d::Model*>(node);
    if (g3dmdl != nullptr) {
      Views_TabTitleFancy(mG3dMdlView, index);
      return;
    }
    auto* j3dmdl = dynamic_cast<riistudio::j3d::Model*>(node);
    if (j3dmdl != nullptr) {
      Views_TabTitleFancy(mJ3dMdlView, index);
      return;
    }
    auto* g3dmat = dynamic_cast<riistudio::g3d::Material*>(node);
    if (g3dmat != nullptr) {
      Views_TabTitleFancy(mG3dMatView, index);
      return;
    }
    auto* j3dmat = dynamic_cast<riistudio::j3d::Material*>(node);
    if (j3dmat != nullptr) {
      Views_TabTitleFancy(mJ3dMatView, index);
      return;
    }
    auto* g3poly = dynamic_cast<g3d::Polygon*>(node);
    if (g3poly != nullptr) {
      Views_TabTitleFancy(mG3dPolyView, index);
      return;
    }
    auto* j3poly = dynamic_cast<j3d::Shape*>(node);
    if (j3poly != nullptr) {
      Views_TabTitleFancy(mJ3dPolyView, index);
      return;
    }
    auto* g3bone = dynamic_cast<g3d::Bone*>(node);
    if (g3bone != nullptr) {
      Views_TabTitleFancy(mG3dBoneView, index);
      return;
    }
    auto* j3bone = dynamic_cast<j3d::Joint*>(node);
    if (j3bone != nullptr) {
      Views_TabTitleFancy(mJ3dBoneView, index);
      return;
    }
    auto* g3dvc = dynamic_cast<riistudio::g3d::ColorBuffer*>(node);
    if (g3dvc != nullptr) {
      Views_TabTitleFancy(mG3dVcView, index);
      return;
    }
    auto* g3dsrt = dynamic_cast<riistudio::g3d::SRT0*>(node);
    if (g3dsrt != nullptr) {
      Views_TabTitleFancy(mG3dSrtView, index);
      return;
    }
    auto* g3tex = dynamic_cast<g3d::Texture*>(node);
    if (g3tex != nullptr) {
      Views_TabTitleFancy(mG3dTexView, index);
      return;
    }
    auto* j3tex = dynamic_cast<j3d::Texture*>(node);
    if (j3tex != nullptr) {
      Views_TabTitleFancy(mG3dTexView, index);
      return;
    }
    ImGui::Text("???");
  }
  bool Tab(int index, std::function<void()> postUpdate,
           std::function<void(const char*)> commit,
           std::function<void()> handleUpdates, kpi::IObject* node,
           auto&& selected,
           std::function<void(const lib3d::Texture*, u32)> drawIcon,
           riistudio::g3d::Model* mdl) {
    auto* g3dmdl = dynamic_cast<g3d::Model*>(node);
    if (g3dmdl != nullptr) {
      auto dl = kpi::MakeDelegate<g3d::Model>(postUpdate, commit, g3dmdl,
                                              selected, drawIcon);
      auto ok = Views_Tab(mG3dMdlView, dl, index);
      handleUpdates();
      return ok;
    }
    auto* j3dmdl = dynamic_cast<j3d::Model*>(node);
    if (j3dmdl != nullptr) {
      auto dl = kpi::MakeDelegate<j3d::Model>(postUpdate, commit, j3dmdl,
                                              selected, drawIcon);
      auto ok = Views_Tab(mJ3dMdlView, dl, index);
      handleUpdates();
      return ok;
    }
    auto* g3dmat = dynamic_cast<riistudio::g3d::Material*>(node);
    if (g3dmat != nullptr) {
      auto dl = kpi::MakeDelegate<riistudio::g3d::Material>(
          postUpdate, commit, g3dmat, selected, drawIcon);
      auto cv = CovariantPD<g3d::Material, libcube::IGCMaterial>::from(dl);
      auto ok = Views_Tab(mG3dMatView, cv, index);
      handleUpdates();
      return ok;
    }
    auto* j3dmat = dynamic_cast<riistudio::j3d::Material*>(node);
    if (j3dmat != nullptr) {
      auto dl = kpi::MakeDelegate<riistudio::j3d::Material>(
          postUpdate, commit, j3dmat, selected, drawIcon);
      auto cv = CovariantPD<j3d::Material, libcube::IGCMaterial>::from(dl);
      auto ok = Views_Tab(mJ3dMatView, cv, index);
      handleUpdates();
      return ok;
    }
    auto* g3poly = dynamic_cast<riistudio::g3d::Polygon*>(node);
    if (g3poly != nullptr) {
      auto dl = kpi::MakeDelegate<riistudio::g3d::Polygon>(
          postUpdate, commit, g3poly, selected, drawIcon);
      auto cv = CovariantPD<g3d::Polygon, libcube::IndexedPolygon>::from(dl);
      auto ok = Views_Tab(mG3dPolyView, cv, index);
      handleUpdates();
      return ok;
    }
    auto* j3poly = dynamic_cast<riistudio::j3d::Shape*>(node);
    if (j3poly != nullptr) {
      auto dl = kpi::MakeDelegate<riistudio::j3d::Shape>(
          postUpdate, commit, j3poly, selected, drawIcon);
      auto cv = CovariantPD<j3d::Shape, libcube::IndexedPolygon>::from(dl);
      auto ok = Views_Tab(mJ3dPolyView, cv, index);
      handleUpdates();
      return ok;
    }
    auto* j3bone = dynamic_cast<riistudio::j3d::Joint*>(node);
    if (j3bone != nullptr) {
      auto dl = kpi::MakeDelegate<riistudio::j3d::Joint>(
          postUpdate, commit, j3bone, selected, drawIcon);
      auto cv = CovariantPD<j3d::Joint, libcube::IBoneDelegate>::from(dl);
      auto ok = Views_Tab(mJ3dBoneView, cv, index);
      handleUpdates();
      return ok;
    }
    auto* g3bone = dynamic_cast<riistudio::g3d::Bone*>(node);
    if (g3bone != nullptr) {
      auto dl = kpi::MakeDelegate<riistudio::g3d::Bone>(
          postUpdate, commit, g3bone, selected, drawIcon);
      auto cv = CovariantPD<g3d::Bone, libcube::IBoneDelegate>::from(dl);
      auto ok = Views_Tab(mG3dBoneView, cv, index);
      handleUpdates();
      return ok;
    }
    auto* g3dv = dynamic_cast<riistudio::g3d::ColorBuffer*>(node);
    if (g3dv != nullptr) {
      auto dl = kpi::MakeDelegate<riistudio::g3d::ColorBuffer>(
          postUpdate, commit, g3dv, selected, drawIcon);
      auto ok = Views_Tab(mG3dVcView, dl, index);
      handleUpdates();
      return ok;
    }
    auto* g3ds = dynamic_cast<riistudio::g3d::SRT0*>(node);
    if (g3ds != nullptr) {
      auto dl = kpi::MakeDelegate<riistudio::g3d::SRT0>(
          postUpdate, commit, g3ds, selected, drawIcon);
      if (!mdl) {
        return false;
      }
      drawProperty(dl, mG3dSrtView.srt, *mdl);
      handleUpdates();
      return true;
    }
    auto* g3tex = dynamic_cast<g3d::Texture*>(node);
    if (g3tex != nullptr) {
      auto dl = kpi::MakeDelegate<g3d::Texture>(postUpdate, commit, g3tex,
                                                selected, drawIcon);
      auto cv = CovariantPD<g3d::Texture, libcube::Texture>::from(dl);
      auto ok = Views_Tab(mG3dTexView, cv, index);
      handleUpdates();
      return ok;
    }
    auto* j3tex = dynamic_cast<j3d::Texture*>(node);
    if (j3tex != nullptr) {
      auto dl = kpi::MakeDelegate<j3d::Texture>(postUpdate, commit, j3tex,
                                                selected, drawIcon);
      auto cv = CovariantPD<j3d::Texture, libcube::Texture>::from(dl);
      auto ok = Views_Tab(mJ3dTexView, cv, index);
      handleUpdates();
      return ok;
    }
    return false;
  }
};

} // namespace riistudio::frontend
