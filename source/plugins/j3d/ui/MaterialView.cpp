#include <frontend/editor/kit/Image.hpp>
#include <kpi/PropertyView.hpp>
#include <plugins/gc/Export/Material.hpp>
#include <vendor/fa5/IconsFontAwesome5.h>
#include <vendor/imgui/imgui.h>
#include <vendor/imgui/imgui_internal.h>

#include <plugins/j3d/Material.hpp>

namespace riistudio::j3d::ui {

struct ConditionalActive {
  ConditionalActive(bool pred) : bPred(pred) {
    if (!bPred) {
      ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
      ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }
  }
  ~ConditionalActive() {
    if (!bPred) {
      ImGui::PopItemFlag();
      ImGui::PopStyleVar();
    }
  }
  bool bPred = false;
};

struct J3DDataSurface final {
  const char *name = "J3D Data";
  const char *icon = ICON_FA_BOXES;
};

void drawProperty(kpi::PropertyDelegate<Material> &delegate, J3DDataSurface) {
  int flag = delegate.getActive().flag;
  ImGui::InputInt("Flag", &flag, 1, 1);
  KPI_PROPERTY_EX(delegate, flag, static_cast<u8>(flag));

  libcube::gx::ColorF32 clr_f32;

  if (ImGui::CollapsingHeader("Fog", ImGuiTreeNodeFlags_DefaultOpen)) {
    auto &fog = delegate.getActive().fogInfo;

    int orthoOrPersp = static_cast<int>(fog.type) >=
                               static_cast<int>(Fog::Type::OrthographicLinear)
                           ? 0
                           : 1;
    int fogType = static_cast<int>(fog.type);
    if (fogType >= static_cast<int>(Fog::Type::OrthographicLinear))
      fogType -= 5;
    ImGui::Combo("Projection", &orthoOrPersp, "Orthographic\0Perspective\0");
    ImGui::Combo("Function", &fogType,
                 "None\0Linear\0Exponential\0Quadratic\0Inverse "
                 "Exponential\0Quadratic\0");
    int new_type = fogType;
    if (new_type != 0)
      new_type += (1 - orthoOrPersp) * 5;
    KPI_PROPERTY_EX(delegate, fogInfo.type, static_cast<Fog::Type>(new_type));
    bool enabled = fog.enabled;
    ImGui::Checkbox("Fog Enabled", &enabled);
    KPI_PROPERTY_EX(delegate, fogInfo.enabled, enabled);

    {
      ConditionalActive g(enabled /* && new_type != 0*/);
      ImGui::PushItemWidth(200);
      {
        int center = fog.center;
        ImGui::InputInt("Center", &center);
        KPI_PROPERTY_EX(delegate, fogInfo.center, static_cast<u16>(center));

        float startZ = fog.startZ;
        ImGui::InputFloat("Start Z", &startZ);
        KPI_PROPERTY_EX(delegate, fogInfo.startZ, startZ);
        ImGui::SameLine();
        float endZ = fog.endZ;
        ImGui::InputFloat("End Z", &endZ);
        KPI_PROPERTY_EX(delegate, fogInfo.endZ, endZ);

        float nearZ = fog.nearZ;
        ImGui::InputFloat("Near Z", &nearZ);
        KPI_PROPERTY_EX(delegate, fogInfo.nearZ, nearZ);
        ImGui::SameLine();
        float farZ = fog.farZ;
        ImGui::InputFloat("Far Z", &farZ);
        KPI_PROPERTY_EX(delegate, fogInfo.farZ, farZ);
      }
      ImGui::PopItemWidth();
      clr_f32 = fog.color;
      ImGui::ColorEdit4("Fog Color", clr_f32);
      KPI_PROPERTY_EX(delegate, fogInfo.color, (libcube::gx::Color)clr_f32);

      // RangeAdjTable? Maybe a graph?
    }
  }

  if (ImGui::CollapsingHeader("Light Colors (Usually ignored by games)",
                              ImGuiTreeNodeFlags_DefaultOpen)) {

    int i = 0;
    for (auto &clr : delegate.getActive().lightColors) {
      clr_f32 = clr;
      ImGui::ColorEdit4(
          (std::string("Light Color ") + std::to_string(i)).c_str(), clr_f32);

      KPI_PROPERTY(delegate, clr, (libcube::gx::Color)clr_f32, lightColors[i]);
      ++i;
    }
  }
  if (ImGui::CollapsingHeader("NBT Scale", ImGuiTreeNodeFlags_DefaultOpen)) {
    bool enabled = delegate.getActive().nbtScale.enable;
    auto scale = delegate.getActive().nbtScale.scale;

    ImGui::Checkbox("NBT Enabled", &enabled);
    KPI_PROPERTY_EX(delegate, nbtScale.enable, enabled);

    {
      ConditionalActive g(enabled);

      ImGui::InputFloat3("Scale", &scale.x);
      KPI_PROPERTY_EX(delegate, nbtScale.scale, scale);
    }
  }
}

void install() {
  kpi::PropertyViewManager::getInstance()
      .addPropertyView<Material, J3DDataSurface>();
}

} // namespace riistudio::j3d::ui