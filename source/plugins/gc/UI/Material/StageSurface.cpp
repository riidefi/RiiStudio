#include "Common.hpp"
#include <core/3d/ui/Image.hpp>        // for ImagePreview
#include <plugins/gc/UI/TevSolver.hpp> // for optimizeNode

#include <plugins/gc/Export/Scene.hpp>

namespace libcube::UI {

using namespace riistudio::util;

struct StageSurface final {
  static inline const char* name = "Stage";
  static inline const char* icon = (const char*)ICON_FA_NETWORK_WIRED;

  // Mark this surface to be more than an IDL tag.
  int tag_stateful;

  riistudio::frontend::ImagePreview mImg; // In mat sampler
  std::string mLastImg;
};

const char* colorOpt =
    "Register 3 Color\0Register 3 Alpha\0Register 0 "
    "Color\0Register 0 Alpha\0Register 1 Color\0Register 1 "
    "Alpha\0Register 2 Color\0Register 2 Alpha\0Texture "
    "Color\0Texture Alpha\0Raster Color\0Raster Alpha\0 1.0\0 "
    "0.5\0 Constant Color Selection\0 0.0\0\0";
const char* alphaOpt = "Register 3 Alpha\0Register 0 Alpha\0Register 1 "
                       "Alpha\0Register 2 Alpha\0Texture Alpha\0Raster "
                       "Alpha\0Constant Alpha Selection\0 0.0\0\0";

template <typename T> T DrawKonstSel(T x) {
  int ksel = static_cast<int>(x);
  bool k_constant = ksel == std::clamp(ksel, static_cast<int>(T::const_8_8),
                                       static_cast<int>(T::const_1_8));
  int k_numerator =
      k_constant ? 8 - (ksel - static_cast<int>(T::const_8_8)) : -1;
  int k_reg = !k_constant ? (ksel - static_cast<int>(T::k0)) % 5 : -1;
  int k_sub = !k_constant ? (ksel - static_cast<int>(T::k0)) / 5
                          : -1; // rgba, r, g, b, a

  int k_type = k_constant ? 0 : 1;
  const int last_k_type = k_type;
  ImGui::Combo("Konst Selection", &k_type, "Constant\0Uniform\0");
  if (k_type == 0) { // constant
    float k_frac = static_cast<float>(k_numerator) / 8.0f;
    ImGui::SliderFloat("Constant Value", &k_frac, 0.0f, 1.0f);
    k_numerator = static_cast<int>(roundf(k_frac * 8.0f));
    k_numerator = std::max(k_numerator, 1);
  } else { // uniform
    if (k_type != last_k_type) {
      k_reg = 0;
      k_sub = 0;
    }
    ImGui::PushItemWidth(ImGui::GetWindowContentRegionWidth() / 3 - 2);
    ImGui::Combo(
        ".##Konstant Register ID", &k_reg,
        "Konst Color 0\0Konst Color 1\0Konst Color 2\0Konst Color 3\0");
    ImGui::SameLine();
    if (std::is_same_v<T, gx::TevKColorSel>) {
      ImGui::Combo("Konst Register##Subscript", &k_sub,
                   "RGB\0RRR\0GGG\0BBB\0AAA\0");
    } else {
      --k_sub;
      ImGui::Combo("Konst Register##Subscript", &k_sub, "R\0G\0B\0A\0");
      ++k_sub;
    }
    ImGui::PopItemWidth();
  }

  if (k_type == 0) {
    k_numerator -= 1;               // [0, 7]
    k_numerator = -k_numerator + 7; // -> [7, 0]
    ksel = static_cast<int>(T::const_8_8) + k_numerator;
  } else {
    ksel = static_cast<int>(T::k0) + (k_sub * 5) + k_reg;
  }
  return static_cast<T>(ksel);
}

void drawProperty(kpi::PropertyDelegate<IGCMaterial>& delegate,
                  StageSurface& tev) {
  auto& matData = delegate.getActive().getMaterialData();
  const libcube::Scene* pScn = dynamic_cast<const libcube::Scene*>(
      dynamic_cast<const kpi::IObject*>(&delegate.getActive())
          ->childOf->childOf);

  auto drawStage = [&](librii::gx::TevStage& stage, int i) {
#define STAGE_PROP(a, b) AUTO_PROP(mStages[i].a, b)
    if (ImGui::CollapsingHeader("Stage Setting",
                                ImGuiTreeNodeFlags_DefaultOpen)) {
      // RasColor
      int rasId = [](gx::ColorSelChanApi sel) -> int {
        switch (sel) {
        case gx::ColorSelChanApi::color0:
        case gx::ColorSelChanApi::alpha0:
        case gx::ColorSelChanApi::color0a0:
          return 0;
        case gx::ColorSelChanApi::color1:
        case gx::ColorSelChanApi::alpha1:
        case gx::ColorSelChanApi::color1a1:
          return 1;
        default:
        case gx::ColorSelChanApi::zero:
        case gx::ColorSelChanApi::null:
          return 2;
        case gx::ColorSelChanApi::ind_alpha:
          return 3;
        case gx::ColorSelChanApi::normalized_ind_alpha:
          return 4;
        }
      }(stage.rasOrder);
      const int rasIdOld = rasId;
      ImGui::Combo("Channel ID", &rasId,
                   "Channel 0\0Channel 1\0None\0Indirect Alpha\0Normalized "
                   "Indirect Alpha\0");

      if (rasId != rasIdOld) {
        gx::ColorSelChanApi ras = [](int sel) -> gx::ColorSelChanApi {
          switch (sel) {
          case 0:
            return gx::ColorSelChanApi::color0a0;
          case 1:
            return gx::ColorSelChanApi::color1a1;
          case 2:
          default:
            return gx::ColorSelChanApi::zero; // TODO: Prefer null?
          case 3:
            return gx::ColorSelChanApi::ind_alpha;
          case 4:
            return gx::ColorSelChanApi::normalized_ind_alpha;
          }
        }(rasId);

        STAGE_PROP(rasOrder, ras);
      }
      {
        ImGui::PushItemWidth(50);
        int ras_swap = stage.rasSwap;
        ImGui::SameLine();
        ImGui::SliderInt("Swap ID##Ras", &ras_swap, 0, 3);
        STAGE_PROP(rasSwap, static_cast<u8>(ras_swap));
        ImGui::PopItemWidth();
      }
      if (stage.texCoord != stage.texMap) {
        ImGui::Text("TODO: TexCoord != TexMap: Not valid");
      } else {
        // TODO: Better selection here
        int texid = stage.texMap;
        texid = SamplerCombo(texid, matData.samplers,
                             delegate.getActive().getTextureSource(*pScn),
                             delegate.mEd, true);
        if (texid != stage.texMap) {
          for (auto* e : delegate.mAffected) {
            auto& stage = e->getMaterialData().mStages[i];
            stage.texCoord = stage.texMap = texid;
          }
          delegate.commit("Property update");
        }

        ImGui::PushItemWidth(50);
        int tex_swap = stage.texMapSwap;
        ImGui::SameLine();
        ImGui::SliderInt("Swap ID##Tex", &tex_swap, 0, 3);
        STAGE_PROP(texMapSwap, static_cast<u8>(tex_swap));
        ImGui::PopItemWidth();
      }
      if (stage.texMap >= matData.texGens.size()) {
        ImGui::Text("No valid image.");
      } else {
        const riistudio::lib3d::Texture* curImg = nullptr;

        const auto mImgs = delegate.getActive().getTextureSource(*pScn);
        for (auto& it : mImgs) {
          if (it.getName() == matData.samplers[stage.texMap]->mTexture) {
            curImg = &it;
          }
        }
        if (matData.samplers[stage.texCoord]->mTexture != tev.mLastImg) {
          tev.mImg.setFromImage(*curImg);
          tev.mLastImg = curImg->getName();
        }
        tev.mImg.draw(128.0f * (static_cast<f32>(curImg->getWidth()) /
                                static_cast<f32>(curImg->getHeight())),
                      128.0f);
      }
    }

    if (ImGui::CollapsingHeader("Color Stage",
                                ImGuiTreeNodeFlags_DefaultOpen)) {
      // TODO: Only add for now..
      if (stage.colorStage.formula == librii::gx::TevColorOp::add) {
        TevExpression expression(stage.colorStage);

        ImGui::SetWindowFontScale(1.3f);
        ImGui::Text("%s", expression.getString());
        ImGui::SetWindowFontScale(1.0f);

        int a = static_cast<int>(stage.colorStage.a);
        int b = static_cast<int>(stage.colorStage.b);
        int c = static_cast<int>(stage.colorStage.c);
        int d = static_cast<int>(stage.colorStage.d);
        bool clamp = stage.colorStage.clamp;
        int bias = static_cast<int>(stage.colorStage.bias);
        int scale = static_cast<int>(stage.colorStage.scale);
        int dst = static_cast<int>(stage.colorStage.out);

        const auto ksel = DrawKonstSel(stage.colorStage.constantSelection);
        STAGE_PROP(colorStage.constantSelection, ksel);

        auto draw_color_operand = [&](const char* title, int* op, u32 op_mask) {
          ConditionalActive g(expression.isUsed(op_mask), false);
          ImGui::Combo(title, op, colorOpt);
        };

        draw_color_operand("Operand A", &a, A);
        draw_color_operand("Operand B", &b, B);
        draw_color_operand("Operand C", &c, C);
        draw_color_operand("Operand D", &d, D);
        ImGui::Combo("Bias", &bias,
                     "No bias\0Add middle gray\0Subtract middle gray\0");
        ImGui::Combo("Scale", &scale,
                     "100% brightness\0"
                     "200% brightness\0"
                     "400% brightness\0"
                     "50% brightness\0");
        ImGui::Checkbox("Clamp calculation to 0-255", &clamp);
        ImGui::Combo("Calculation Result Output Destionation", &dst,
                     "Register 3\0Register 0\0Register 1\0Register 2\0");

        STAGE_PROP(colorStage.a, static_cast<gx::TevColorArg>(a));
        STAGE_PROP(colorStage.b, static_cast<gx::TevColorArg>(b));
        STAGE_PROP(colorStage.c, static_cast<gx::TevColorArg>(c));
        STAGE_PROP(colorStage.d, static_cast<gx::TevColorArg>(d));
        STAGE_PROP(colorStage.clamp, clamp);
        STAGE_PROP(colorStage.bias, static_cast<gx::TevBias>(bias));
        STAGE_PROP(colorStage.scale, static_cast<gx::TevScale>(scale));
        STAGE_PROP(colorStage.out, static_cast<gx::TevReg>(dst));
      }
    }
    if (ImGui::CollapsingHeader("Alpha Stage",
                                ImGuiTreeNodeFlags_DefaultOpen)) {
      IDScope alphag("Alpha");
      if (stage.alphaStage.formula == librii::gx::TevAlphaOp::add) {
        TevExpression expression(stage.alphaStage);

        ImGui::SetWindowFontScale(1.3f);
        ImGui::Text("%s", expression.getString());
        ImGui::SetWindowFontScale(1.0f);

        int a = static_cast<int>(stage.alphaStage.a);
        int b = static_cast<int>(stage.alphaStage.b);
        int c = static_cast<int>(stage.alphaStage.c);
        int d = static_cast<int>(stage.alphaStage.d);
        bool clamp = stage.alphaStage.clamp;
        int bias = static_cast<int>(stage.alphaStage.bias);
        int scale = static_cast<int>(stage.alphaStage.scale);
        int dst = static_cast<int>(stage.alphaStage.out);

        const auto ksel = DrawKonstSel(stage.alphaStage.constantSelection);
        STAGE_PROP(alphaStage.constantSelection, ksel);

        auto draw_alpha_operand = [&](const char* title, int* op, u32 op_mask) {
          ConditionalActive g(expression.isUsed(op_mask), false);
          ImGui::Combo(title, op, alphaOpt);
        };

        draw_alpha_operand("Operand A##Alpha", &a, A);
        draw_alpha_operand("Operand B##Alpha", &b, B);
        draw_alpha_operand("Operand C##Alpha", &c, C);
        draw_alpha_operand("Operand D##Alpha", &d, D);
        ImGui::Combo("Bias##Alpha", &bias,
                     "No bias\0Add middle gray\0Subtract middle gray\0");
        ImGui::Combo("Scale", &scale, "* 1\0* 2\0* 4\0");
        ImGui::Checkbox("Clamp calculation to 0-255##Alpha", &clamp);
        ImGui::Combo("Calculation Result Output Destionation##Alpha", &dst,
                     "Register 3\0Register 0\0Register 1\0Register 2\0");

        STAGE_PROP(alphaStage.a, static_cast<gx::TevAlphaArg>(a));
        STAGE_PROP(alphaStage.b, static_cast<gx::TevAlphaArg>(b));
        STAGE_PROP(alphaStage.c, static_cast<gx::TevAlphaArg>(c));
        STAGE_PROP(alphaStage.d, static_cast<gx::TevAlphaArg>(d));
        STAGE_PROP(alphaStage.clamp, clamp);
        STAGE_PROP(alphaStage.bias, static_cast<gx::TevBias>(bias));
        STAGE_PROP(alphaStage.scale, static_cast<gx::TevScale>(scale));
        STAGE_PROP(alphaStage.out, static_cast<gx::TevReg>(dst));
      }
    }
  };

  std::array<bool, 16> opened{true};

  auto& stages = matData.mStages;

  if (ImGui::Button("Add a Stage")) {
    stages.push_back({});
  }

  if (ImGui::BeginTabBar("Stages",
                         ImGuiTabBarFlags_AutoSelectNewTabs |
                             ImGuiTabBarFlags_FittingPolicyResizeDown)) {
    for (std::size_t i = 0; i < stages.size(); ++i) {
      auto& stage = stages[i];

      opened[i] = true;

      if (opened[i] && ImGui::BeginTabItem(
                           (std::string("Stage ") + std::to_string(i)).c_str(),
                           &opened[i], ImGuiTabItemFlags_NoPushId)) {
        drawStage(stage, i);

        ImGui::EndTabItem();
      }
    }
    ImGui::EndTabBar();
  }

  bool changed = false;

  for (std::size_t i = 0; i < stages.size(); ++i) {
    if (opened[i])
      continue;

    changed = true;

    // Only one may be deleted at a time
    stages.erase(i);

    if (stages.empty()) {
      stages.push_back({});
    }

    break;
  }

  if (changed)
    delegate.commit("Stage added/removed");
}

kpi::RegisterPropertyView<IGCMaterial, StageSurface> StageSurfaceInstaller;

} // namespace libcube::UI
