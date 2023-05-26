#include "G3dUi.hpp"

namespace riistudio::g3d::ui {

void drawProperty(kpi::PropertyDelegate<Material>& delegate, G3DDataSurface) {
  auto* gm = &delegate.getActive();
  bool used = gm->fogIndex >= 0;
  bool input = ImGui::Checkbox("Use scene fog?", &used);
  riistudio::util::ConditionalActive ca(used);
  int fog = std::max<int>(0, gm->fogIndex);
  if (!used) {
    fog = -1;
  }
  ImGui::SameLine();
  input |= ImGui::InputInt("index", &fog);
  fog = std::min<int>(127, fog);
  if (!used) {
    fog = -1;
  }
  if (input && gm->fogIndex != fog) {
    for (auto& a : delegate.mAffected) {
      if (auto* g = dynamic_cast<riistudio::g3d::Material*>(a)) {
        g->fogIndex = fog;
        g->onUpdate();
      }
    }
    delegate.commit("Fog update");
  }
}

void drawProperty(kpi::PropertyDelegate<Texture>& delegate, G3DTexDataSurface) {
  bool custom_lod = delegate.getActive().custom_lod;
  ImGui::Checkbox("Custom Quality Range", &custom_lod);
  KPI_PROPERTY_EX(delegate, custom_lod, custom_lod);

  riistudio::util::ConditionalActive g(custom_lod);

  float min = delegate.getActive().minLod;
  ImGui::InputFloat("Maximum mipmap quality", &min);
  KPI_PROPERTY_EX(delegate, minLod, min);

  float max = delegate.getActive().maxLod;
  ImGui::InputFloat("Maximum mipmap quality", &max);
  KPI_PROPERTY_EX(delegate, maxLod, max);
}

void drawProperty(kpi::PropertyDelegate<Model>& delegate, G3DMdlDataSurface) {
  auto& active = delegate.getActive();
  auto [nVert, nTri] = librii::gx::computeVertTriCounts(active.getMeshes());
  ImGui::Text("Number of facepoints: %u\n", nVert);
  ImGui::Text("Number of faces: %u\n", nTri);
}

} // namespace riistudio::g3d::ui
