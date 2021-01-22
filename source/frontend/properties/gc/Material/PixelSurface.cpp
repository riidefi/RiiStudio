#include "Common.hpp"
#include <imcxx/Widgets.hpp>
#include <librii/hx/PixMode.hpp>

namespace libcube::UI {

using namespace riistudio::util;

struct PixelSurface final {
  static inline const char* name = "Pixel";
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
    ImGui::Text(
        "This material is currently in the default BrawlBox configuration.");
    ImGui::Text("This is considerably less performant than the Opaque "
                "configuration while being visually identical.\nIt is strongly "
                "recommended that you convert the material.");
    if (ImGui::Button("Convert Material"))
      pix_mode = librii::hx::PIX_DEFAULT_OPAQUE;
    ImGui::SetWindowFontScale(1.0f);
  } else {
    ImGui::Combo("Configuration", &pix_mode,
                 "Opaque\0Stencil Alpha\0Translucent\0Custom\0");
  }
  if (pix_mode_before != pix_mode) {
    if (pix_mode == librii::hx::PIX_CUSTOM) {
      surface.force_custom_whole = matData.name;
    } else {
      for (auto* pO : delegate.mAffected) {
        librii::hx::configurePixMode(
            pO->getMaterialData(), static_cast<librii::hx::PixMode>(pix_mode));

        pO->notifyObservers();
      }
      delegate.commit("Updated pixel config");

      surface.force_custom_whole.clear();
      surface.force_custom_at.clear();
    }
  }

  if (pix_mode != librii::hx::PIX_CUSTOM)
    return;
  if (ImGui::CollapsingHeader("Scenegraph", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Combo(
        "Render Pass", &xlu_mode,
        "Pass 0: Opaque objects, no order\0Pass 1: Translucent objects, "
        "sorted front-to-back (reverse painter's algorithm)\0");
  }
  delegate.property(
      delegate.getActive().isXluPass(), xlu_mode != 0,
      [&](const auto& x) { return x.isXluPass(); },
      [&](auto& x, bool y) {
        x.setXluPass(y);
        x.notifyObservers();
      });

  if (ImGui::CollapsingHeader("Alpha Comparison",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    const auto alpha_test_before = alpha_test;
    ImGui::Combo("Alpha Test", &alpha_test, "Custom\0Disabled\0Stencil\0");
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
      ImGui::Text("Pixels are either fully transparent or fully opaque.");
    } else {
      const char* compStr =
          "Always do not pass.\0<\0==\0<=\0>\0!=\0>=\0Always pass.\0";
      ImGui::PushItemWidth(100);

      {
        ImGui::Text("( Pixel Alpha");
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
        ImGui::Text(")");
      }
      {
        auto op =
            imcxx::Combo("##o", matData.alphaCompare.op, "&&\0||\0!=\0==\0");
        AUTO_PROP(alphaCompare.op, op);
      }
      {
        ImGui::Text("( Pixel Alpha");
        ImGui::SameLine();
        const auto rightAlpha =
            imcxx::Combo("##r", matData.alphaCompare.compRight, compStr);
        AUTO_PROP(alphaCompare.compRight, rightAlpha);

        int rightRef = static_cast<int>(matData.alphaCompare.refRight);
        ImGui::SameLine();
        ImGui::SliderInt("##rr", &rightRef, 0, 255);
        AUTO_PROP(alphaCompare.refRight, (u8)rightRef);

        ImGui::SameLine();
        ImGui::Text(")");
      }
      ImGui::PopItemWidth();
    }
  }

  if (ImGui::CollapsingHeader("Z Buffer", ImGuiTreeNodeFlags_DefaultOpen)) {
    bool zcmp = matData.zMode.compare;
    ImGui::Checkbox("Compare Z Values", &zcmp);
    AUTO_PROP(zMode.compare, zcmp);

    {
      ConditionalActive g(zcmp);

      ImGui::Indent(30.0f);

      bool zearly = matData.earlyZComparison;
      ImGui::Checkbox("Compare Before Texture Processing", &zearly);
      AUTO_PROP(earlyZComparison, zearly);

      bool zwrite = matData.zMode.update;
      ImGui::Checkbox("Write to Z Buffer", &zwrite);
      AUTO_PROP(zMode.update, zwrite);

      auto zcond = imcxx::Combo(
          "Condition", matData.zMode.function,
          "Never draw.\0Pixel Z < EFB Z\0Pixel Z == EFB Z\0Pixel Z <= "
          "EFB Z\0Pixel Z > EFB Z\0Pixel Z != EFB Z\0Pixel Z >= EFB "
          "Z\0 Always draw.\0");
      AUTO_PROP(zMode.function, zcond);

      ImGui::Unindent(30.0f);
    }
  }
  if (ImGui::CollapsingHeader("Blending", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::PushItemWidth(200);
    auto btype = imcxx::Combo(
        "Type", matData.blendMode.type,
        "Do not blend.\0Blending\0Logical Operations\0Subtract from "
        "Frame Buffer\0");
    AUTO_PROP(blendMode.type, btype);

    {

      ConditionalActive g(btype == librii::gx::BlendModeType::blend);
      ImGui::Text("Blend calculation");

      const char* blendOpts =
          " 0\0 1\0 EFB Color\0 1 - EFB Color\0 Pixel Alpha\0 1 - Pixel "
          "Alpha\0 EFB Alpha\0 1 - EFB Alpha\0";
      const char* blendOptsDst =
          " 0\0 1\0 Pixel Color\0 1 - Pixel Color\0 Pixel Alpha\0 1 - Pixel "
          "Alpha\0 EFB Alpha\0 1 - EFB Alpha\0";
      ImGui::Text("( Pixel Color * ");

      ImGui::SameLine();
      auto srcFact = imcxx::Combo("##Src", matData.blendMode.source, blendOpts);
      AUTO_PROP(blendMode.source, srcFact);

      ImGui::SameLine();
      ImGui::Text(") + ( EFB Color * ");

      ImGui::SameLine();
      auto dstFact =
          imcxx::Combo("##Dst", matData.blendMode.dest, blendOptsDst);
      AUTO_PROP(blendMode.dest, dstFact);

      ImGui::SameLine();
      ImGui::Text(")");
    }
    {
      ConditionalActive g(btype == librii::gx::BlendModeType::logic);
      ImGui::Text("Logical Operations");
    }
    ImGui::PopItemWidth();
  }
}

kpi::RegisterPropertyView<IGCMaterial, PixelSurface> PixelSurfaceInstaller;

} // namespace libcube::UI
