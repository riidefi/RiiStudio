#include "BlightEditor.hpp"

#include <glm/gtc/type_ptr.hpp>

namespace riistudio::frontend {

void BlightEditorPropertyGrid::DrawLobj(
    librii::egg::Light& cfg, std::span<const librii::gx::Color> ambs) {
  ImGui::Checkbox("Enable Calc", &cfg.enable_calc);
  ImGui::SameLine();
  riistudio::util::ConditionalActive g(cfg.enable_calc);
  ImGui::Checkbox("Send to GPU", &cfg.enable_sendToGpu);
  ImGui::SameLine();

  ImGui::Separator();
  cfg.lightType = imcxx::EnumCombo("Type", cfg.lightType);
  if (cfg.hasPosition() || true) {
    ImGui::InputFloat3("Position", glm::value_ptr(cfg.position));
  }
  if (cfg.hasAim()) {
    ImGui::InputFloat3("Aim", glm::value_ptr(cfg.aim));
  }
  bool snap = cfg.snapTargetIndex.has_value();
  ImGui::Checkbox("Snap To Other Light", &snap);
  if (snap) {
    int v = cfg.snapTargetIndex.value_or(0);
    ImGui::InputInt("Other Light Index", &v);
    cfg.snapTargetIndex = v;
  } else {
    cfg.snapTargetIndex.reset();
  }
  ImGui::Separator();

  ImGui::Checkbox("BLMAP", &cfg.blmap);
  ImGui::SameLine();
  ImGui::Checkbox("Affects BRRES materials (Color)", &cfg.brresColor);
  ImGui::SameLine();
  ImGui::Checkbox("Affects BRRES materials (Alpha)", &cfg.brresAlpha);

  ImGui::Separator();
  {
    riistudio::util::ConditionalActive g(cfg.hasDistAttn());
    // if (ImGui::BeginChild("Distance Falloff"))
    {
      ImGui::InputFloat("Reference Distance", &cfg.refDist);
      if (ImGui::SliderFloat("Reference Brightness", &cfg.refBright, 0.0f,
                             1.0f)) {
        if (cfg.refBright >= 1.0f) {
          cfg.refBright = std::nextafterf(1.0f, 0.0f);

        } else if (cfg.refBright <= 0.0f) {
          cfg.refBright = std::nextafterf(0.0f, 1.0f);
        }
      }
      cfg.distAttnFunction = imcxx::EnumCombo("Funtion", cfg.distAttnFunction);
    }
  }
  ImGui::Separator();
  {
    riistudio::util::ConditionalActive g(cfg.hasAngularAttn());
    // if (ImGui::BeginChild("Angular Falloff",ImVec2(0, 0), true))
    {
      if (ImGui::SliderFloat("Spotlight Radius", &cfg.spotCutoffAngle, 0.0f,
                             90.0f)) {
        if (cfg.refBright > 90.0f) {
          cfg.refBright = 90.0f;

        } else if (cfg.refBright <= 0.0f) {
          cfg.refBright = std::nextafterf(0.0f, 90.0f);
        }
      }
      cfg.spotFunction = imcxx::EnumCombo("Spotlight Type", cfg.spotFunction);
    }
  }
  ImGui::Separator();

  {
    librii::gx::ColorF32 fc = cfg.color;
    ImGui::ColorEdit4("Color", fc);
    cfg.color = fc;
  }
  ImGui::InputFloat("Color Scale", &cfg.intensity);
  {
    librii::gx::ColorF32 fc = cfg.specularColor;
    ImGui::ColorEdit4("Specular Color", fc);
    cfg.specularColor = fc;
  }
  {
    int amb = cfg.ambientLightIndex;
    ImGui::InputInt("Ambient Light Index", &amb);
    if (amb < 0 || amb >= ambs.size()) {
      amb = cfg.ambientLightIndex;
    }
    librii::gx::ColorF32 fc = ambs[amb];
    ImGui::ColorEdit4("Ambient Color (at that index)", fc,
                      ImGuiColorEditFlags_NoOptions |
                          ImGuiColorEditFlags_DisplayRGB);
    // amb_updated |= ambs[amb] != static_cast<librii::gx::Color>(fc);
    cfg.ambientLightIndex = amb;
  }
  {
    int unk = cfg.miscFlags;
    ImGui::InputInt("Misc Flags", &unk);
    cfg.miscFlags = unk;
  }

  ImGui::Separator();
  ImGui::Checkbox(
      "Override DISTANCE+ANGULAR FALLOFF settings at runtime with SHININESS=16",
      &cfg.runtimeShininess);

  ImGui::Checkbox("Override DISTANCE FALLOFF settings at runtime",
                  &cfg.runtimeDistAttn);
  ImGui::Checkbox("Override ANGULAR FALLOFF settings at runtime",
                  &cfg.runtimeAngularAttn);
  ImGui::Separator();
  cfg.coordSpace = imcxx::EnumCombo("Coordinate Space", cfg.coordSpace);
}
void BlightEditorPropertyGrid::DrawAmb(librii::gx::Color& cfg_) {
  librii::gx::ColorF32 fc = cfg_;
  ImGui::ColorEdit4("Color", fc);
  cfg_ = fc;
}

} // namespace riistudio::frontend
