#pragma once

#include <vendor/cista.h>

#include <frontend/properties/g3d/G3dSrtView.hpp>
#include <frontend/properties/g3d/G3dUi.hpp>
#include <frontend/properties/g3d/G3dUniformAnimView.hpp>
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
#include <frontend/properties/gc/ShaderView.hpp>
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
#ifdef BUILD_DEBUG
  libcube::UI::ShaderSurface shader;
#endif
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
#ifdef BUILD_DEBUG
  libcube::UI::ShaderSurface shader;
#endif
};

struct G3dPolygonViews {
  libcube::UI::PolyDataSurface index_data;
  libcube::UI::PolyDescriptorSurface vertex_descriptor;
};
struct J3dPolygonViews {
  libcube::UI::PolyDataSurface index_data;
  libcube::UI::PolyDescriptorSurface vertex_descriptor;
  riistudio::j3d::ui::ShapeJ3DSurface shape_j3d;
};

struct G3dBoneViews {
  libcube::UI::BoneDisplaySurface displays;
  libcube::UI::BoneTransformSurface transformation;
};

struct J3dBoneViews {
  libcube::UI::BoneDisplaySurface displays;
  libcube::UI::BoneTransformSurface transformation;
  riistudio::j3d::ui::BoneJ3DSurface bone_j3d;
};

struct G3dSrtViews {
  // NOTE: This struct is incompatible with Views_Tabs as a model reference is
  // required!
  riistudio::g3d::G3dSrtOptionsSurface srt;
};
struct G3dVcViews {
  riistudio::g3d::G3dVertexColorDataSurface data;
  riistudio::g3d::G3dVertexColorQuantSurface quant;
};

struct G3dTexViews {
  libcube::UI::ImageSurface image;
  riistudio::g3d::ui::G3DTexDataSurface tds;
};
struct J3dTexViews {
  libcube::UI::ImageSurface image;
};
struct G3dMdlViews {
  riistudio::g3d::ui::G3DMdlDataSurface ms;
};
struct J3dMdlViews {
  riistudio::j3d::ui::ModelJ3DSurface ms;
};

struct G3dUniformAnimViews {
  riistudio::g3d::G3dUniformAnimDataSurface data;
};

// TODO:
// [X] BoneJ3DSurface
// [X] ShapeJ3DSurface
// [X] ModelJ3DSurface
// [X] G3DMdlDataSurface
// [X] G3DTexDataSurface

std::vector<std::string> Views_TabTitles(auto&& view) {
  std::vector<std::string> titles;
  cista::for_each_field(view, [&](auto&& x) {
    titles.push_back(std::format("{} {}", x.icon, x.name()));
  });
  return titles;
}

template <typename Q> class has_icon_color {
  typedef char YesType[1];
  typedef char NoType[2];

  template <typename C> static YesType& test(decltype(&C::color));
  template <typename C> static NoType& test(...);

public:
  enum { value = sizeof(test<Q>(0)) == sizeof(YesType) };
};

struct Title {
  ImVec4 iconColor = Clr(0xff'ff'ff);
  std::string icon;
  std::string name;
};
template <typename Y> static inline Title GetTitle(const Y& x) {
  Title title;
  title.icon = x.icon;
  if constexpr (has_icon_color<Y>::value) {
    title.iconColor = x.color;
  }
  title.name = x.name();
  return title;
}

bool Views_TabTitleFancy(auto&& view, int index) {
  int counter = 0;
  bool ok = false;
  cista::for_each_field(view, [&](auto&& x) {
    if (counter == index) {
      auto title = GetTitle(x);
      ImGui::TextColored(title.iconColor, "%s", title.icon.c_str());
      ImGui::SameLine();
      ImGui::Text(" %s", title.name.c_str());
      ok = true;
    }
    ++counter;
  });
  return ok;
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
