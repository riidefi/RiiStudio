#include "BfgEditor.hpp"

#include <librii/gx.h>

namespace riistudio::frontend {

void BfgEditorPropertyGrid::Draw(librii::egg::BFG::Entry& fog) {
  int orthoOrPersp =
      static_cast<int>(fog.mType) >=
              static_cast<int>(librii::gx::FogType::OrthographicLinear)
          ? 0
          : 1;
  int fogType = static_cast<int>(fog.mType);
  if (fogType >= static_cast<int>(librii::gx::FogType::OrthographicLinear))
    fogType -= 5;
  ImGui::Combo("Projection", &orthoOrPersp, "Orthographic\0Perspective\0");
  ImGui::Combo("Function", &fogType,
               "None\0Linear\0Exponential\0Quadratic\0Inverse "
               "Exponential\0Quadratic\0");
  int new_type = fogType;
  if (new_type != 0)
    new_type += (1 - orthoOrPersp) * 5;
  fog.mType = new_type;

  bool enabled = fog.mEnabled;
  ImGui::Checkbox("Fog Enabled", &enabled);
  fog.mEnabled = enabled;

  {
    util::ConditionalActive g(enabled);
    ImGui::InputFloat("Start Z", &fog.mStartZ);

    ImGui::InputFloat("End Z", &fog.mEndZ);

	ImVec4 tmp = ImGui::ColorConvertU32ToFloat4(fog.mColor);
    ImGui::ColorEdit4("Fog Color", &tmp.x);
    fog.mColor = ImGui::ColorConvertFloat4ToU32(tmp);

    int center = fog.mCenter;
    ImGui::InputInt("Center", &center);
    fog.mCenter = center;

    ImGui::SliderFloat("Fade Speed", &fog.mFadeSpeed, 0.0f, 1.0f);
  }
}

} // namespace riistudio::frontend
