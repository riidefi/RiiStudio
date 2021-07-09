#include "Common.hpp"

namespace libcube::UI {

using namespace riistudio::util;

struct LightingSurface final {
  static inline const char* name() { return "Lighting"_j; }
  static inline const char* icon = (const char*)ICON_FA_SUN;
};

void drawProperty(kpi::PropertyDelegate<IGCMaterial>& delegate,
                  LightingSurface) {
  auto& matData = delegate.getActive().getMaterialData();

  // Color0Alpha0, Color1Alpha1
  auto& colors = matData.chanData;
  // Color0, Alpha0, Color1, Alpha1
  auto& controls = matData.colorChanControls;

  // ImGui::Text("Number of colors:   %u", colors.size());
  // ImGui::Text("Number of controls: %u", controls.size());

  if (colors.size() > controls.size() / 2 || controls.size() % 2 != 0 ||
      controls.size() == 0) {
    ImGui::Text("Cannot edit this material's lighting data."_j);
    return;
  }

  const auto draw_color_channel = [&](int i) {
    auto& ctrl = controls[i];

    ImGui::CollapsingHeader(i % 2 ? "Alpha Channel"_j : "Color Channel"_j,
                            ImGuiTreeNodeFlags_DefaultOpen);

    {
      auto diffuse_src = ctrl.Material;
      bool vclr = diffuse_src == gx::ColorSource::Vertex;
      ImGui::Checkbox(i % 2 ? "Vertex Alpha"_j : "Vertex Colors"_j, &vclr);
      diffuse_src = vclr ? gx::ColorSource::Vertex : gx::ColorSource::Register;
      AUTO_PROP(colorChanControls[i].Material, diffuse_src);

      {
        ConditionalActive g(!vclr);

        librii::gx::ColorF32 mclr = colors[i / 2].matColor;
        if (i % 2 == 0) {
          ImGui::ColorEdit3("Diffuse Color"_j, mclr);
        } else {
          ImGui::DragFloat("Diffuse Alpha"_j, &mclr.a, 0.0f, 1.0f);
        }
        AUTO_PROP(chanData[i / 2].matColor, (librii::gx::Color)mclr);
      }
    }
    bool enabled = ctrl.enabled;
    ImGui::Checkbox("Affected by Light", &enabled);
    AUTO_PROP(colorChanControls[i].enabled, enabled);

    ConditionalActive g(enabled);
    {
      auto amb_src = ctrl.Ambient;
      bool vclr = amb_src == gx::ColorSource::Vertex;
      ImGui::Checkbox(i % 2 ? "Ambient Alpha uses Vertex Alpha"_j
                            : "Ambient Color uses Vertex Colors"_j,
                      &vclr);
      amb_src = vclr ? gx::ColorSource::Vertex : gx::ColorSource::Register;
      AUTO_PROP(colorChanControls[i].Ambient, amb_src);

      {
        ConditionalActive g(!vclr);

        librii::gx::ColorF32 aclr = colors[i / 2].ambColor;
        if (i % 2 == 0) {
          ImGui::ColorEdit3("Ambient Color"_j, aclr);
        } else {
          ImGui::DragFloat("Ambient Alpha"_j, &aclr.a, 0.0f, 1.0f);
        }
        AUTO_PROP(chanData[i / 2].ambColor, (librii::gx::Color)aclr);
      }
    }

    int diffuse_fn = static_cast<int>(ctrl.diffuseFn);
    int atten_fn = static_cast<int>(ctrl.attenuationFn);

    ImGui::Combo("Diffusion Type"_j, &diffuse_fn,
                 "None\0"
                 "Signed\0"
                 "Clamped\0");
    AUTO_PROP(colorChanControls[i].diffuseFn,
              static_cast<librii::gx::DiffuseFunction>(diffuse_fn));
    ImGui::Combo("Attenuation Type"_j, &atten_fn,
                 "Specular\0"
                 "Spotlight (Diffuse)\0"
                 "None\0"
                 "None*\0");
    AUTO_PROP(colorChanControls[i].attenuationFn,
              static_cast<librii::gx::AttenuationFunction>(atten_fn));

    ImGui::Text("Enabled Lights:"_j);
    auto light_mask = static_cast<u32>(ctrl.lightMask);
    ImGui::BeginTable("Light Mask"_j, 8, ImGuiTableFlags_Borders);
    for (int i = 0; i < 8; ++i) {
      char header[2]{0};
      header[0] = '0' + i;
      ImGui::TableSetupColumn(header);
    }
    ImGui::TableAutoHeaders();
    ImGui::TableNextRow();
    for (int i = 0; i < 8; ++i) {
      bool light_enabled = (static_cast<u32>(ctrl.lightMask) & (1 << i)) != 0;
      ImGui::TableSetColumnIndex(i);
      ImGui::PushID(i);
      ImGui::Checkbox("###TableLightMask", &light_enabled);
      ImGui::PopID();

      if (light_enabled) {
        light_mask |= (1 << i);
      } else {
        light_mask &= ~(1 << i);
      }
    }
    ImGui::EndTable();
    AUTO_PROP(colorChanControls[i].lightMask,
              static_cast<librii::gx::LightID>(light_mask));
  };

  for (int i = 0; i < controls.size(); i += 2) {
    IDScope g(i);
    if (ImGui::CollapsingHeader(
            (std::string("Channel "_j) + std::to_string(i / 2)).c_str(),
            ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Columns(2);
      {
        IDScope g(i);
        draw_color_channel(i);
      }
      ImGui::NextColumn();
      {
        IDScope g(i + 1);
        draw_color_channel(i + 1);
      }
      ImGui::EndColumns();
    }
  }
}

kpi::RegisterPropertyView<IGCMaterial, LightingSurface>
    LightingSurfaceInstaller;

} // namespace libcube::UI
