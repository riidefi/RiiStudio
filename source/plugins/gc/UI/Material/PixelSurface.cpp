#include "Common.hpp"

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

  enum AlphaTest : int {
    ALPHA_TEST_CUSTOM,
    ALPHA_TEST_DISABLED,
    ALPHA_TEST_STENCIL,
  };
  int alpha_test = ALPHA_TEST_CUSTOM;

  const gx::AlphaComparison stencil_comparison = {
      .compLeft = gx::Comparison::GEQUAL,
      .refLeft = 128,
      .op = gx::AlphaOp::_and,
      .compRight = gx::Comparison::LEQUAL,
      .refRight = 255};
  const gx::AlphaComparison disabled_comparison = {
      .compLeft = gx::Comparison::ALWAYS,
      .refLeft = 0,
      .op = gx::AlphaOp::_and,
      .compRight = gx::Comparison::ALWAYS,
      .refRight = 0};

  if (surface.force_custom_at != matData.name) {
    if (matData.alphaCompare == stencil_comparison)
      alpha_test = ALPHA_TEST_STENCIL;
    else if (matData.alphaCompare == disabled_comparison)
      alpha_test = ALPHA_TEST_DISABLED;
  }
  const gx::BlendMode no_blend = {.type = gx::BlendModeType::none,
                                  .source = gx::BlendModeFactor::src_a,
                                  .dest = gx::BlendModeFactor::inv_src_a,
                                  .logic = gx::LogicOp::_copy};
  const gx::BlendMode yes_blend = {.type = gx::BlendModeType::blend,
                                   .source = gx::BlendModeFactor::src_a,
                                   .dest = gx::BlendModeFactor::inv_src_a,
                                   .logic = gx::LogicOp::_copy};

  const gx::ZMode normal_z = {
      .compare = true, .function = gx::Comparison::LEQUAL, .update = true};
  const gx::ZMode blend_z = {
      .compare = true, .function = gx::Comparison::LEQUAL, .update = false};

  enum PixMode : int {
    PIX_DEFAULT_OPAQUE,
    PIX_STENCIL,
    PIX_TRANSLUCENT,
    PIX_CUSTOM,
    PIX_BBX_DEFAULT
  };
  int pix_mode = PIX_CUSTOM;

  int xlu_mode = delegate.getActive().isXluPass() ? 1 : 0;

  if (surface.force_custom_whole != matData.name) {
    if (alpha_test == ALPHA_TEST_DISABLED && matData.zMode == normal_z &&
        matData.blendMode == no_blend && xlu_mode == 0) {
      pix_mode =
          matData.earlyZComparison ? PIX_DEFAULT_OPAQUE : PIX_BBX_DEFAULT;
    } else if (alpha_test == ALPHA_TEST_STENCIL && matData.zMode == normal_z &&
               matData.blendMode == no_blend && xlu_mode == 0 &&
               !matData.earlyZComparison) {
      pix_mode = PIX_STENCIL;
    } else if (alpha_test == ALPHA_TEST_DISABLED && matData.zMode == blend_z &&
               matData.blendMode == yes_blend && xlu_mode == 1 &&
               matData.earlyZComparison) {
      pix_mode = PIX_TRANSLUCENT;
    }
  }

  const auto pix_mode_before = pix_mode;
  if (pix_mode == PIX_BBX_DEFAULT) {
    ImGui::SetWindowFontScale(1.2f);
    ImGui::Text(
        "This material is currently in the default BrawlBox configuration.");
    ImGui::Text("This is considerably less performant than the Opaque "
                "configuration while being visually identical.\nIt is strongly "
                "recommended that you convert the material.");
    if (ImGui::Button("Convert Material"))
      pix_mode = PIX_DEFAULT_OPAQUE;
    ImGui::SetWindowFontScale(1.0f);
  } else {
    ImGui::Combo("Configuration", &pix_mode,
                 "Opaque\0Stencil Alpha\0Translucent\0Custom\0");
  }
  if (pix_mode_before != pix_mode) {
    if (pix_mode == PIX_CUSTOM) {
      surface.force_custom_whole = matData.name;
    } else {
      for (auto* pO : delegate.mAffected) {
        auto& o = pO->getMaterialData();

        if (pix_mode == PIX_DEFAULT_OPAQUE) {
          o.alphaCompare = disabled_comparison;
          o.zMode = normal_z;
          o.blendMode = no_blend;
          pO->setXluPass(false);
          o.earlyZComparison = true;
        } else if (pix_mode == PIX_STENCIL) {
          o.alphaCompare = stencil_comparison;
          o.zMode = normal_z;
          o.blendMode = no_blend;
          pO->setXluPass(false);
          o.earlyZComparison = false;
        } else if (pix_mode == PIX_TRANSLUCENT) {
          o.alphaCompare = disabled_comparison;
          o.zMode = blend_z;
          o.blendMode = yes_blend;
          pO->setXluPass(true);
          o.earlyZComparison = true;
        }

        pO->notifyObservers();
      }
      delegate.commit("Updated pixel config");

      surface.force_custom_whole.clear();
      surface.force_custom_at.clear();
    }
  }

  if (pix_mode != PIX_CUSTOM)
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
      if (alpha_test == ALPHA_TEST_CUSTOM) {
        surface.force_custom_at = matData.name;
      } else {
        if (alpha_test == ALPHA_TEST_DISABLED)
          AUTO_PROP(alphaCompare, disabled_comparison);
        else if (alpha_test == ALPHA_TEST_STENCIL)
          AUTO_PROP(alphaCompare, stencil_comparison);
        surface.force_custom_at.clear();
      }
    }
    if (alpha_test == ALPHA_TEST_DISABLED) {
      // ImGui::Text("There is no alpha test");
    } else if (alpha_test == ALPHA_TEST_STENCIL) {
      ImGui::Text("Pixels are either fully transparent or fully opaque.");
    } else {
      const char* compStr =
          "Always do not pass.\0<\0==\0<=\0>\0!=\0>=\0Always pass.\0";
      ImGui::PushItemWidth(100);

      {
        ImGui::Text("( Pixel Alpha");
        ImGui::SameLine();
        int leftAlpha = static_cast<int>(matData.alphaCompare.compLeft);
        ImGui::Combo("##l", &leftAlpha, compStr);
        AUTO_PROP(alphaCompare.compLeft,
                  static_cast<librii::gx::Comparison>(leftAlpha));

        int leftRef = static_cast<int>(
            delegate.getActive().getMaterialData().alphaCompare.refLeft);
        ImGui::SameLine();
        ImGui::SliderInt("##lr", &leftRef, 0, 255);
        AUTO_PROP(alphaCompare.refLeft, static_cast<u8>(leftRef));

        ImGui::SameLine();
        ImGui::Text(")");
      }
      {
        int op = static_cast<int>(matData.alphaCompare.op);
        ImGui::Combo("##o", &op, "&&\0||\0!=\0==\0");
        AUTO_PROP(alphaCompare.op, static_cast<librii::gx::AlphaOp>(op));
      }
      {
        ImGui::Text("( Pixel Alpha");
        ImGui::SameLine();
        int rightAlpha = static_cast<int>(matData.alphaCompare.compRight);
        ImGui::Combo("##r", &rightAlpha, compStr);
        AUTO_PROP(alphaCompare.compRight,
                  static_cast<librii::gx::Comparison>(rightAlpha));

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

      int zcond = static_cast<int>(matData.zMode.function);
      ImGui::Combo("Condition", &zcond,
                   "Never draw.\0Pixel Z < EFB Z\0Pixel Z == EFB Z\0Pixel Z <= "
                   "EFB Z\0Pixel Z > EFB Z\0Pixel Z != EFB Z\0Pixel Z >= EFB "
                   "Z\0 Always draw.\0");
      AUTO_PROP(zMode.function, static_cast<librii::gx::Comparison>(zcond));

      ImGui::Unindent(30.0f);
    }
  }
  if (ImGui::CollapsingHeader("Blending", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::PushItemWidth(200);
    int btype = static_cast<int>(matData.blendMode.type);
    ImGui::Combo("Type", &btype,
                 "Do not blend.\0Blending\0Logical Operations\0Subtract from "
                 "Frame Buffer\0");
    AUTO_PROP(blendMode.type, static_cast<librii::gx::BlendModeType>(btype));

    {

      ConditionalActive g(btype ==
                          static_cast<int>(librii::gx::BlendModeType::blend));
      ImGui::Text("Blend calculation");

      const char* blendOpts =
          " 0\0 1\0 EFB Color\0 1 - EFB Color\0 Pixel Alpha\0 1 - Pixel "
          "Alpha\0 EFB Alpha\0 1 - EFB Alpha\0";
      const char* blendOptsDst =
          " 0\0 1\0 Pixel Color\0 1 - Pixel Color\0 Pixel Alpha\0 1 - Pixel "
          "Alpha\0 EFB Alpha\0 1 - EFB Alpha\0";
      ImGui::Text("( Pixel Color * ");

      int srcFact = static_cast<int>(matData.blendMode.source);
      ImGui::SameLine();
      ImGui::Combo("##Src", &srcFact, blendOpts);
      AUTO_PROP(blendMode.source,
                static_cast<librii::gx::BlendModeFactor>(srcFact));

      ImGui::SameLine();
      ImGui::Text(") + ( EFB Color * ");

      int dstFact = static_cast<int>(matData.blendMode.dest);
      ImGui::SameLine();
      ImGui::Combo("##Dst", &dstFact, blendOptsDst);
      AUTO_PROP(blendMode.dest,
                static_cast<librii::gx::BlendModeFactor>(dstFact));

      ImGui::SameLine();
      ImGui::Text(")");
    }
    {
      ConditionalActive g(btype ==
                          static_cast<int>(librii::gx::BlendModeType::logic));
      ImGui::Text("Logical Operations");
    }
    ImGui::PopItemWidth();
  }
}

kpi::RegisterPropertyView<IGCMaterial, PixelSurface>
    PixelSurfaceInstaller;

} // namespace libcube::UI
