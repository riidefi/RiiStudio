#pragma once

#include <frontend/properties/gc/Material/Common.hpp>
#include <frontend/widgets/Image.hpp> // ImagePreview
#include <imcxx/Widgets.hpp>          // imcxx::Combo
#include <librii/hx/KonstSel.hpp>     // elevateKonstSel
#include <librii/tev/TevSolver.hpp>   // optimizeNode
#include <plugins/gc/Export/Scene.hpp>

namespace libcube::UI {

struct StageSurface final {
  static inline const char* name() { return "Stage"_j; }
  static inline const char* icon = (const char*)ICON_FA_NETWORK_WIRED;
  static inline ImVec4 color = Clr(0xb5e2fa);

  // Mark this surface to be more than an IDL tag.
  int tag_stateful;

  riistudio::frontend::ImagePreview mImg; // In mat sampler
  std::string mLastImg;
};
void drawProperty(kpi::PropertyDelegate<IGCMaterial>& delegate,
                  StageSurface& tev);

} // namespace libcube::UI
