#include "Common.hpp"
#include <frontend/properties/gc/TevSolver.hpp> // optimizeNode
#include <frontend/widgets/Image.hpp>           // ImagePreview
#include <imcxx/Widgets.hpp>                    // imcxx::Combo
#include <librii/hx/KonstSel.hpp>               // elevateKonstSel
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

const char* colorOpt = "Register 3 Color\0"
                       "Register 3 Alpha\0"
                       "Register 0 Color\0"
                       "Register 0 Alpha\0"
                       "Register 1 Color\0"
                       "Register 1 Alpha\0"
                       "Register 2 Color\0"
                       "Register 2 Alpha\0"
                       "Texture Color\0"
                       "Texture Alpha\0"
                       "Raster Color\0"
                       "Raster Alpha\0"
                       "1.0\0"
                       "0.5\0"
                       "Constant Color Selection\0"
                       "0.0\0"
                       "\0";

const char* alphaOpt = "Register 3 Alpha\0"
                       "Register 0 Alpha\0"
                       "Register 1 Alpha\0"
                       "Register 2 Alpha\0"
                       "Texture Alpha\0"
                       "Raster Alpha\0"
                       "Constant Alpha Selection\0"
                       "0.0\0"
                       "\0";

librii::gx::TevBias drawTevBias(librii::gx::TevBias bias) {
  return imcxx::Combo("Bias", bias,
                      "No bias\0"
                      "Add middle gray\0"
                      "Subtract middle gray\0");
}
librii::gx::TevScale drawTevScale(librii::gx::TevScale scale) {
  return imcxx::Combo("Scale", scale,
                      "100% brightness\0"
                      "200% brightness\0"
                      "400% brightness\0"
                      "50% brightness\0");
}
librii::gx::TevReg drawOutRegister(librii::gx::TevReg out) {
  return imcxx::Combo("Calculation Result Output Destionation", out,
                      "Register 3\0"
                      "Register 0\0"
                      "Register 1\0"
                      "Register 2\0");
}
gx::TevColorArg drawTevOp(const TevExpression& expression, const char* title,
                          gx::TevColorArg op, u32 op_mask) {
  ConditionalHighlight g(expression.isUsed(op_mask));
  return imcxx::Combo(title, op, colorOpt);
}
gx::TevAlphaArg drawTevOp(const TevExpression& expression, const char* title,
                          gx::TevAlphaArg op, u32 op_mask) {
  ConditionalHighlight g(expression.isUsed(op_mask));
  return imcxx::Combo(title, op, alphaOpt);
};

template <typename T> T drawSubStage(T stage) {
  TevExpression expression(stage);

  ImGui::SetWindowFontScale(1.3f);
  ImGui::Text("%s", expression.getString());
  ImGui::SetWindowFontScale(1.0f);

  stage.constantSelection = DrawKonstSel(stage.constantSelection);

  stage.a = drawTevOp(expression, "Operand A", stage.a, A);
  stage.b = drawTevOp(expression, "Operand B", stage.b, B);
  stage.c = drawTevOp(expression, "Operand C", stage.c, C);
  stage.d = drawTevOp(expression, "Operand D", stage.d, D);

  stage.bias = drawTevBias(stage.bias);
  stage.scale = drawTevScale(stage.scale);

  ImGui::Checkbox("Clamp calculation to 0-255", &stage.clamp);
  stage.out = drawOutRegister(stage.out);

  return stage;
}

template <typename T> T DrawKonstSel(T x) {
  auto ksel = librii::hx::elevateKonstSel(x);

  int k_type = ksel.k_constant ? 0 : 1;
  const int last_k_type = k_type;
  ImGui::Combo("Konst Selection", &k_type, "Constant\0Uniform\0");
  if (k_type == 0) { // constant
    float k_frac = static_cast<float>(ksel.k_numerator) / 8.0f;
    ImGui::SliderFloat("Constant Value", &k_frac, 0.0f, 1.0f);
    ksel.k_numerator = static_cast<int>(roundf(k_frac * 8.0f));
    ksel.k_numerator = std::max(ksel.k_numerator, 1);
  } else { // uniform
    if (k_type != last_k_type) {
      ksel.k_reg = 0;
      ksel.k_sub = 0;
    }
    ImGui::PushItemWidth(ImGui::GetWindowContentRegionWidth() / 3 - 2);
    ImGui::Combo(
        ".##Konstant Register ID", &ksel.k_reg,
        "Konst Color 0\0Konst Color 1\0Konst Color 2\0Konst Color 3\0");
    ImGui::SameLine();
    if (std::is_same_v<T, gx::TevKColorSel>) {
      ImGui::Combo("Konst Register##Subscript", &ksel.k_sub,
                   "RGB\0RRR\0GGG\0BBB\0AAA\0");
    } else {
      --ksel.k_sub;
      ImGui::Combo("Konst Register##Subscript", &ksel.k_sub, "R\0G\0B\0A\0");
      ++ksel.k_sub;
    }
    ImGui::PopItemWidth();
  }
  ksel.k_constant = k_type == 0;

  return librii::hx::lowerKonstSel<T>(ksel);
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
                   "Channel 0\0"
                   "Channel 1\0"
                   "None\0"
                   "Indirect Alpha\0"
                   "Normalized Indirect Alpha\0");

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
          if (it.getName() == matData.samplers[stage.texMap].mTexture) {
            curImg = &it;
          }
        }
        if (curImg) {
          if (matData.samplers[stage.texCoord].mTexture != tev.mLastImg) {
            tev.mImg.setFromImage(*curImg);
            tev.mLastImg = curImg->getName();
          }
          tev.mImg.draw(128.0f * (static_cast<f32>(curImg->getWidth()) /
                                  static_cast<f32>(curImg->getHeight())),
                        128.0f);
        }
      }
    }

    if (ImGui::CollapsingHeader("Color Stage",
                                ImGuiTreeNodeFlags_DefaultOpen)) {
      // TODO: Only add for now..
      if (stage.colorStage.formula == librii::gx::TevColorOp::add) {
        librii::gx::TevStage::ColorStage substage =
            drawSubStage(stage.colorStage);
        STAGE_PROP(colorStage.constantSelection, substage.constantSelection);
        STAGE_PROP(colorStage.a, substage.a);
        STAGE_PROP(colorStage.b, substage.b);
        STAGE_PROP(colorStage.c, substage.c);
        STAGE_PROP(colorStage.d, substage.d);
        STAGE_PROP(colorStage.clamp, substage.clamp);
        STAGE_PROP(colorStage.bias, substage.bias);
        STAGE_PROP(colorStage.scale, substage.scale);
        STAGE_PROP(colorStage.out, substage.out);
      }
    }
    if (ImGui::CollapsingHeader("Alpha Stage",
                                ImGuiTreeNodeFlags_DefaultOpen)) {
      IDScope alphag("Alpha");
      if (stage.alphaStage.formula == librii::gx::TevAlphaOp::add) {
        librii::gx::TevStage::AlphaStage substage =
            drawSubStage(stage.alphaStage);

        STAGE_PROP(alphaStage.constantSelection, substage.constantSelection);
        STAGE_PROP(alphaStage.a, substage.a);
        STAGE_PROP(alphaStage.b, substage.b);
        STAGE_PROP(alphaStage.c, substage.c);
        STAGE_PROP(alphaStage.d, substage.d);
        STAGE_PROP(alphaStage.clamp, substage.clamp);
        STAGE_PROP(alphaStage.bias, substage.bias);
        STAGE_PROP(alphaStage.scale, substage.scale);
        STAGE_PROP(alphaStage.out, substage.out);
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
