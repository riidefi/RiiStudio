#include <core/kpi/Node2.hpp>
#include <core/kpi/PropertyView.hpp>
#include <core/util/gui.hpp>
#include <imcxx/Widgets.hpp>
#include <plugins/g3d/collection.hpp>
#include <vendor/fa5/IconsFontAwesome5.h>

namespace riistudio::g3d {

bool Checkbox(const char* name, bool init) {
  ImGui::Checkbox(name, &init);
  return init;
}

librii::g3d::SrtFlags DrawSrtFlags(const librii::g3d::SrtFlags& flags) {
  auto result = flags;

  result.Unknown = Checkbox("Unknown"_j, flags.Unknown);
  result.Unknown = Checkbox("Identity Sclaing"_j, flags.Unknown);
  result.Unknown = Checkbox("Identity Rotation"_j, flags.Unknown);
  result.Unknown = Checkbox("Identity Translation"_j, flags.Unknown);
  result.Unknown = Checkbox("Isotropic Sclaing"_j, flags.Unknown);

  return result;
}

librii::g3d::SrtAnimationArchive
DrawSrtOptions(const librii::g3d::SrtAnimationArchive& init) {
  auto result = init;

  int frame_duration = result.frame_duration;
  ImGui::InputInt("Frame duration"_j, &frame_duration);
  result.frame_duration = frame_duration;

  int xform_model = result.xform_model;
  ImGui::Combo("Transform model"_j, &xform_model,
               "Maya\0"
               "XSI\0"
               "Max\0"_j);
  result.xform_model = xform_model;

  result.anim_wrap_mode =
      imcxx::Combo("Temporal wrap mode"_j, result.anim_wrap_mode,
                   "Clamp\0"
                   "Repeat\0"_j);

  return result;
}

struct G3dSrtOptionsSurface {
  static inline const char* name() { return "Options"_j; }
  static inline const char* icon = (const char*)ICON_FA_COG;
};

void drawProperty(kpi::PropertyDelegate<SRT0>& dl, G3dSrtOptionsSurface) {
  auto& srt = dl.getActive();

  // TODO: Can be expensive to copy
  auto edited = DrawSrtOptions(srt);

  KPI_PROPERTY_EX(dl, frame_duration, edited.frame_duration);
  KPI_PROPERTY_EX(dl, xform_model, edited.xform_model);
  KPI_PROPERTY_EX(dl, anim_wrap_mode, edited.anim_wrap_mode);
}

kpi::DecentralizedInstaller VColorInstaller([](kpi::ApplicationPlugins&) {
  auto& inst = kpi::PropertyViewManager::getInstance();
  inst.addPropertyView<SRT0, G3dSrtOptionsSurface>();
});

} // namespace riistudio::g3d
