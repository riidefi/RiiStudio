#include "Common.hpp"
#include <imcxx/Widgets.hpp>
#include <librii/hx/PixMode.hpp>
#include <plugins/g3d/collection.hpp>

namespace libcube::UI {

using namespace riistudio::util;

struct PixelSurface final {
  static inline const char* name() { return "Pixel"_j; }
  static inline const char* icon = (const char*)ICON_FA_GHOST;

  int tag_stateful;

  std::string force_custom_at; // If empty, disabled: Name of material
  std::string force_custom_whole;
};

void drawProperty(kpi::PropertyDelegate<IGCMaterial>& delegate,
                  PixelSurface& surface) {
  auto& matData = delegate.getActive().getMaterialData();

  int alpha_test = librii::hx::ALPHA_TEST_CUSTOM;

  if (surface.force_custom_at != matData.name) {
    alpha_test = librii::hx::classifyAlphaCompare(matData.alphaCompare);
  }

  int pix_mode = librii::hx::PIX_CUSTOM;

  int xlu_mode = matData.xlu ? 1 : 0;

  if (surface.force_custom_whole != matData.name) {
    pix_mode = librii::hx::classifyPixMode(matData);
  }

  const auto pix_mode_before = pix_mode;
  if (pix_mode == librii::hx::PIX_BBX_DEFAULT) {
    ImGui::SetWindowFontScale(1.2f);
    ImGui::TextUnformatted(
        "This material is currently in the default BrawlBox configuration."_j);
    ImGui::TextUnformatted(
        "This is considerably less performant than the Opaque "
        "configuration while being visually identical.\nIt is strongly "
        "recommended that you convert the material."_j);
    if (ImGui::Button("Convert Material"_j))
      pix_mode = librii::hx::PIX_DEFAULT_OPAQUE;
    ImGui::SetWindowFontScale(1.0f);
  } else {
    ImGui::Combo("Configuration"_j, &pix_mode,
                 "Opaque\0"
                 "Stencil Alpha\0"
                 "Translucent\0"
                 "Harry Potter Effect: Stencil Variant\0"
                 "Harry Potter Effect: Translucent Variant\0"
                 "Custom\0"_j);
  }
  if (pix_mode_before != pix_mode) {
    if (pix_mode == librii::hx::PIX_CUSTOM) {
      surface.force_custom_whole = matData.name;
    } else {
      for (auto* pO : delegate.mAffected) {
        librii::hx::configurePixMode(
            pO->getMaterialData(), static_cast<librii::hx::PixMode>(pix_mode));

        pO->onUpdate();
      }
      delegate.commit("Updated pixel config");

      surface.force_custom_whole.clear();
      surface.force_custom_at.clear();
    }
  }

  if (pix_mode != librii::hx::PIX_CUSTOM)
    goto DstAlpha;
  if (ImGui::CollapsingHeader("Scenegraph"_j, ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Combo("Render Pass"_j, &xlu_mode,
                 "Pass 0: Opaque objects, no order\0"
                 "Pass 1: Translucent objects, sorted front-to-back (reverse "
                 "painter's algorithm)\0"_j);
  }
  delegate.property(
      delegate.getActive().isXluPass(), xlu_mode != 0,
      [&](const auto& x) { return x.isXluPass(); },
      [&](auto& x, bool y) {
        x.setXluPass(y);
        x.onUpdate();
      });

  if (ImGui::CollapsingHeader("Alpha Comparison"_j,
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    const auto alpha_test_before = alpha_test;
    ImGui::Combo("Alpha Test"_j, &alpha_test,
                 "Custom\0"
                 "Disabled\0"
                 "Stencil\0"_j);
    if (alpha_test_before != alpha_test) {
      if (alpha_test == librii::hx::ALPHA_TEST_CUSTOM) {
        surface.force_custom_at = matData.name;
      } else {
        if (alpha_test == librii::hx::ALPHA_TEST_DISABLED)
          AUTO_PROP(alphaCompare, librii::hx::disabled_comparison);
        else if (alpha_test == librii::hx::ALPHA_TEST_STENCIL)
          AUTO_PROP(alphaCompare, librii::hx::stencil_comparison);
        surface.force_custom_at.clear();
      }
    }
    if (alpha_test == librii::hx::ALPHA_TEST_DISABLED) {
      // ImGui::Text("There is no alpha test");
    } else if (alpha_test == librii::hx::ALPHA_TEST_STENCIL) {
      ImGui::TextUnformatted(
          "Pixels are either fully transparent or fully opaque."_j);
    } else {
      const char* compStr = "Always do not pass.\0"
                            "<\0"
                            "==\0"
                            "<=\0"
                            ">\0"
                            "!=\0"
                            ">=\0"
                            "Always pass.\0"_j;
      ImGui::PushItemWidth(100);

      {
        ImGui::TextUnformatted("( Pixel Alpha"_j);
        ImGui::SameLine();
        const auto leftAlpha =
            imcxx::Combo("##l", matData.alphaCompare.compLeft, compStr);
        AUTO_PROP(alphaCompare.compLeft, leftAlpha);

        int leftRef = static_cast<int>(
            delegate.getActive().getMaterialData().alphaCompare.refLeft);
        ImGui::SameLine();
        ImGui::SliderInt("##lr", &leftRef, 0, 255);
        AUTO_PROP(alphaCompare.refLeft, static_cast<u8>(leftRef));

        ImGui::SameLine();
        ImGui::TextUnformatted(")");
      }
      {
        auto op = imcxx::Combo("##o", matData.alphaCompare.op,
                               "&&\0"
                               "||\0"
                               "!=\0"
                               "==\0");
        AUTO_PROP(alphaCompare.op, op);
      }
      {
        ImGui::TextUnformatted("( Pixel Alpha"_j);
        ImGui::SameLine();
        const auto rightAlpha =
            imcxx::Combo("##r", matData.alphaCompare.compRight, compStr);
        AUTO_PROP(alphaCompare.compRight, rightAlpha);

        int rightRef = static_cast<int>(matData.alphaCompare.refRight);
        ImGui::SameLine();
        ImGui::SliderInt("##rr", &rightRef, 0, 255);
        AUTO_PROP(alphaCompare.refRight, (u8)rightRef);

        ImGui::SameLine();
        ImGui::TextUnformatted(")");
      }
      ImGui::PopItemWidth();
    }
  }

  if (ImGui::CollapsingHeader("Z Buffer"_j, ImGuiTreeNodeFlags_DefaultOpen)) {
    bool zcmp = matData.zMode.compare;
    ImGui::Checkbox("Compare Z Values"_j, &zcmp);
    AUTO_PROP(zMode.compare, zcmp);

    {
      ConditionalActive g(zcmp);

      ImGui::Indent(30.0f);

      bool zearly = matData.earlyZComparison;
      ImGui::Checkbox("Compare Before Texture Processing"_j, &zearly);
      AUTO_PROP(earlyZComparison, zearly);

      bool zwrite = matData.zMode.update;
      ImGui::Checkbox("Write to Z Buffer", &zwrite);
      AUTO_PROP(zMode.update, zwrite);

      auto zcond = imcxx::Combo("Condition"_j, matData.zMode.function,
                                "Never draw.\0"
                                "Pixel Z < EFB Z\0"
                                "Pixel Z == EFB Z\0"
                                "Pixel Z <= EFB Z\0"
                                "Pixel Z > EFB Z\0"
                                "Pixel Z != EFB Z\0"
                                "Pixel Z >= EFB Z\0"
                                "Always draw.\0"_j);
      AUTO_PROP(zMode.function, zcond);

      ImGui::Unindent(30.0f);
    }
  }
  if (ImGui::CollapsingHeader("Blending"_j, ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::PushItemWidth(200);
    auto btype = imcxx::Combo("Type"_j, matData.blendMode.type,
                              "Do not blend.\0"
                              "Blending\0"
                              "Logical Operations\0"
                              "Subtract from Frame Buffer\0"_j);
    AUTO_PROP(blendMode.type, btype);

    {

      ConditionalActive g(btype == librii::gx::BlendModeType::blend);
      ImGui::TextUnformatted("Blend calculation"_j);

      const char* blendOpts = "0\0"
                              "1\0"
                              "EFB Color\0"
                              "1 - EFB Color\0"
                              "Pixel Alpha\0"
                              "1 - Pixel Alpha\0"
                              "EFB Alpha\0"
                              "1 - EFB Alpha\0"_j;
      const char* blendOptsDst = "0\0"
                                 "1\0"
                                 "Pixel Color\0"
                                 "1 - Pixel Color\0"
                                 "Pixel Alpha\0"
                                 "1 - Pixel Alpha\0"
                                 "EFB Alpha\0"
                                 "1 - EFB Alpha\0"_j;
      ImGui::TextUnformatted("( Pixel Color * "_j);

      ImGui::SameLine();
      auto srcFact = imcxx::Combo("##Src", matData.blendMode.source, blendOpts);
      AUTO_PROP(blendMode.source, srcFact);

      ImGui::SameLine();
      ImGui::TextUnformatted(") + ( EFB Color * "_j);

      ImGui::SameLine();
      auto dstFact =
          imcxx::Combo("##Dst", matData.blendMode.dest, blendOptsDst);
      AUTO_PROP(blendMode.dest, dstFact);

      ImGui::SameLine();
      ImGui::TextUnformatted(")");
    }
    {
      ConditionalActive g(btype == librii::gx::BlendModeType::logic);
      ImGui::TextUnformatted("Logical Operations (Unsupported)"_j);
    }
    ImGui::PopItemWidth();
  }
DstAlpha:
  if (auto* gm =
          dynamic_cast<riistudio::g3d::Material*>(&delegate.getActive())) {
    bool dstAlphaEnabled = gm->dstAlpha.enabled;
    int dstAlphaValue = gm->dstAlpha.alpha;

    if (ImGui::CollapsingHeader("Destination Alpha",
                                ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Checkbox("Enabled", &dstAlphaEnabled);
      {
        riistudio::util::ConditionalActive c(dstAlphaEnabled);

        ImGui::InputInt("Value", &dstAlphaValue);
      }
      // Force shader recompilation
      for (auto& mat : delegate.mAffected) {
        assert(mat);
        if (mat->getMaterialData().dstAlpha.enabled != dstAlphaEnabled ||
            mat->getMaterialData().dstAlpha.alpha != dstAlphaValue) {
          mat->nextGenerationId();
        }
      }
      AUTO_PROP(dstAlpha.enabled, dstAlphaEnabled);
      AUTO_PROP(dstAlpha.alpha, static_cast<u8>(dstAlphaValue));
    }
  }
}

kpi::RegisterPropertyView<IGCMaterial, PixelSurface> PixelSurfaceInstaller;

} // namespace libcube::UI
