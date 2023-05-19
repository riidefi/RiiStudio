#pragma once

#include <vendor/cista.h>

#include <frontend/properties/g3d/G3dSrtView.hpp>
#include <frontend/properties/g3d/G3dUi.hpp>
#include <frontend/properties/g3d/G3dVertexColorView.hpp>
#include <frontend/properties/gc/Bone/BoneDisplaySurface.hpp>
#include <frontend/properties/gc/Bone/BoneTransformSurface.hpp>
#include <frontend/properties/gc/Material/ColorSurface.hpp>
#include <frontend/properties/gc/Material/DisplaySurface.hpp>
#include <frontend/properties/gc/Material/Indirect.hpp>
#include <frontend/properties/gc/Material/LightingSurface.hpp>
#include <frontend/properties/gc/Material/PixelSurface.hpp>
#include <frontend/properties/gc/Material/SamplerSurface.hpp>
#include <frontend/properties/gc/Material/StageSurface.hpp>
#include <frontend/properties/gc/Material/SwapTableSurface.hpp>
#include <frontend/properties/gc/Polygon/PolyDataSurface.hpp>
#include <frontend/properties/gc/Polygon/PolyDescriptorSurface.hpp>
#include <frontend/properties/gc/TexImageView.hpp>
#include <frontend/properties/j3d/MaterialView.hpp>

namespace riistudio {

template <typename DERIVED, typename PARENT>
struct CovariantPD : public kpi::PropertyDelegate<DERIVED> {
  static_assert(std::is_base_of_v<PARENT, DERIVED>);
  operator kpi::PropertyDelegate<PARENT>&() {
    return *reinterpret_cast<kpi::PropertyDelegate<PARENT>*>(this);
  }
  static CovariantPD& from(kpi::PropertyDelegate<DERIVED>& dl) {
    return reinterpret_cast<CovariantPD&>(dl);
  }
};

struct G3dMaterialViews {
  libcube::UI::ColorSurface colors;
  libcube::UI::IndirectSurface displacement;
  riistudio::g3d::ui::G3DDataSurface fog;
  libcube::UI::LightingSurface lighting;
  libcube::UI::PixelSurface pixel;
  libcube::UI::SwapTableSurface swizzling;
  libcube::UI::StageSurface stage;
  libcube::UI::SamplerSurface samplers;
  libcube::UI::DisplaySurface surface_visibility;
};
struct J3dMaterialViews {
  libcube::UI::ColorSurface colors;
  libcube::UI::IndirectSurface displacement;
  riistudio::j3d::ui::J3DDataSurface j3d_data;
  libcube::UI::LightingSurface lighting;
  libcube::UI::PixelSurface pixel;
  libcube::UI::SwapTableSurface swizzling;
  libcube::UI::StageSurface stage;
  libcube::UI::SamplerSurface samplers;
  libcube::UI::DisplaySurface surface_visibility;
};

struct GcPolygonViews {
  libcube::UI::PolyDataSurface index_data;
  libcube::UI::PolyDescriptorSurface vertex_descriptor;
};

struct GcBoneViews {
  libcube::UI::BoneDisplaySurface displays;
  libcube::UI::BoneTransformSurface transformation;
};

struct G3dSrtViews {
  riistudio::g3d::G3dSrtOptionsSurface srt;
};
struct G3dVcViews {
  riistudio::g3d::G3dVertexColorDataSurface data;
  riistudio::g3d::G3dVertexColorQuantSurface quant;
};

struct GcTexViews {
  libcube::UI::ImageSurface image;
};

std::vector<std::string> Views_TabTitles(auto&& view) {
  std::vector<std::string> titles;
  cista::for_each_field(view, [&](auto&& x) {
    titles.push_back(std::format("{} {}", x.icon, x.name()));
  });
  return titles;
}
bool Views_Tab(auto&& view, auto&& dl, int index) {
  int counter = 0;
  bool ok = false;
  cista::for_each_field(view, [&](auto&& x) {
    if (counter == index) {
      drawProperty(dl, x);
      ok = true;
    }
    ++counter;
  });
  return ok;
}

} // namespace riistudio
