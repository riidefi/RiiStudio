#include "Common.hpp"
#include <frontend/widgets/Image.hpp> // for ImagePreview
#include <imcxx/Widgets.hpp>
#include <librii/hx/TexGenType.hpp>
#include <librii/hx/TextureFilter.hpp>
#include <plugins/gc/Export/Scene.hpp>
#include <plugins/j3d/Material.hpp>

namespace libcube::UI {

void addSampler(libcube::GCMaterialData& d) {
  if (d.texGens.size() != d.samplers.size() || d.texGens.size() == 10 ||
      d.samplers.size() == 8 || d.texMatrices.size() == 10) {
    printf("[Warning] Cannot add sampler on material %s\n", d.name.c_str());
    return;
  }

  gx::TexCoordGen gen;
  gen.setMatrixIndex(d.texMatrices.size());
  d.texGens.push_back(gen);
  d.texMatrices.push_back(GCMaterialData::TexMatrix{});
  d.samplers.push_back(std::make_unique<GCMaterialData::SamplerData>());
}

void deleteSampler(libcube::GCMaterialData& matData, size_t i) {
  // Only one may be deleted at a time
  matData.samplers.erase(i);
  assert(matData.texGens[i].getMatrixIndex() == i);
  matData.texGens.erase(i);
  matData.texMatrices.erase(i);

  // Correct stages
  for (auto& stage : matData.mStages) {
    if (stage.texCoord == stage.texMap) {
      if (stage.texCoord == i) {
        // Might be a better way of doing this
        stage.texCoord = stage.texMap = 0;
      } else if (stage.texCoord > i) {
        --stage.texCoord;
        --stage.texMap;
      }
    }
  }
}

void addSampler(kpi::PropertyDelegate<libcube::IGCMaterial>& delegate) {
  for (auto* obj : delegate.mAffected) {
    addSampler(obj->getMaterialData());
  }
  delegate.commit("Added sampler");
}

using namespace riistudio::util;

struct SamplerSurface final {
  static inline const char* name = "Samplers";
  static inline const char* icon = (const char*)ICON_FA_IMAGES;

  // Mark this surface to be more than an IDL tag.
  int tag_stateful;

  riistudio::frontend::ImagePreview mImg; // In mat sampler
  std::string mLastImg;
};

void drawSamplerImage(const kpi::ConstCollectionRange<libcube::Texture>& mImgs,
                      libcube::GCMaterialData::SamplerData& samp,
                      libcube::UI::SamplerSurface& surface) {
  const riistudio::lib3d::Texture* curImg = nullptr;

  for (auto& it : mImgs) {
    if (it.getName() == samp.mTexture) {
      curImg = &it;
    }
  }

  if (curImg == nullptr) {
    ImGui::Text("No valid image.");
  } else {
    if (surface.mLastImg != curImg->getName()) {
      surface.mImg.setFromImage(*curImg);
      surface.mLastImg = curImg->getName();
    }
    const auto aspect_ratio = static_cast<f32>(curImg->getWidth()) /
                              static_cast<f32>(curImg->getHeight());
    surface.mImg.draw(128.0f * aspect_ratio, 128.0f);
  }
}

void drawProperty(kpi::PropertyDelegate<IGCMaterial>& delegate,
                  SamplerSurface& surface) {
  auto& matData = delegate.getActive().getMaterialData();

  if (matData.texGens.size() != matData.samplers.size()) {
    ImGui::Text("Cannot edit: source data is invalid!");
    return;
  }

  if (ImGui::Button("Add Sampler")) {
    addSampler(delegate);
  }

  if (ImGui::BeginTabBar("Textures")) {
    llvm::SmallVector<bool, 16> open(matData.texGens.size(), true);
    for (std::size_t i = 0; i < matData.texGens.size(); ++i) {
      auto& tg = matData.texGens[i];

      bool identitymatrix = tg.isIdentityMatrix();
      int texmatrixid = tg.getMatrixIndex();

      GCMaterialData::TexMatrix* tm = nullptr;
      if (tg.matrix != gx::TexMatrix::Identity)
        tm = &matData.texMatrices[texmatrixid]; // TODO: Proper lookup
      auto& samp = matData.samplers[i];
      const libcube::Scene* pScn = dynamic_cast<const libcube::Scene*>(
          dynamic_cast<const kpi::IObject*>(&delegate.getActive())
              ->childOf->childOf);
      const auto mImgs = delegate.getActive().getTextureSource(*pScn);
      if (ImGui::BeginTabItem((std::string("Texture ") + std::to_string(i) +
                               " [" + samp->mTexture + "]")
                                  .c_str(),
                              &open[i])) {
        if (ImGui::CollapsingHeader("Image", ImGuiTreeNodeFlags_DefaultOpen)) {
          if (auto result = TextureImageCombo(samp->mTexture.c_str(), mImgs,
                                              delegate.mEd);
              !result.empty()) {
            AUTO_PROP(samplers[i]->mTexture, result);
          }

          if (samp != nullptr)
            drawSamplerImage(mImgs, *samp, surface);
        }
        if (ImGui::CollapsingHeader("Mapping",
                                    ImGuiTreeNodeFlags_DefaultOpen)) {
          if (ImGui::BeginTabBar("Mapping")) {
            if (ImGui::BeginTabItem("Standard")) {
              ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Advanced")) {
              if (ImGui::CollapsingHeader("Texture Coordinate Generator",
                                          ImGuiTreeNodeFlags_DefaultOpen)) {
                librii::hx::TexGenType hx_tg =
                    librii::hx::elevateTexGenType(tg.func);

                ImGui::Combo(
                    "Function", &hx_tg.basefunc,
                    "Standard Texture Matrix\0Bump Mapping: Use vertex "
                    "lighting calculation result.\0SRTG: Map R(ed) and G(reen) "
                    "components of a color channel to U/V coordinates\0");
                {
                  ConditionalActive g(hx_tg.basefunc == 0);
                  ImGui::Combo("Matrix Size", &hx_tg.mtxtype,
                               "UV Matrix: 2x4\0UVW Matrix: 3x4\0");

                  ImGui::Checkbox("Identity Matrix", &identitymatrix);
                  ImGui::SameLine();
                  {
                    ConditionalActive g2(!identitymatrix);
                    ImGui::SliderInt("Matrix ID", &texmatrixid, 0, 7);
                  }
                  librii::gx::TexMatrix newtexmatrix =
                      identitymatrix
                          ? librii::gx::TexMatrix::Identity
                          : static_cast<librii::gx::TexMatrix>(
                                static_cast<int>(
                                    librii::gx::TexMatrix::TexMatrix0) +
                                std::max(0, texmatrixid) * 3);
                  AUTO_PROP(texGens[i].matrix, newtexmatrix);
                }
                {
                  ConditionalActive g(hx_tg.basefunc == 1);
                  ImGui::SliderInt("Hardware light ID", &hx_tg.lightid, 0, 7);
                }

                librii::gx::TexGenType newfunc =
                    librii::hx::lowerTexGenType(hx_tg);
                AUTO_PROP(texGens[i].func, newfunc);

                int src = static_cast<int>(tg.sourceParam);
                ImGui::Combo(
                    "Source data", &src,
                    "Position\0Normal\0Binormal\0Tangent\0UV 0\0UV 1\0UV 2\0UV "
                    "3\0UV 4\0UV 5\0UV 6\0UV 7\0Bump UV0\0Bump UV1\0Bump "
                    "UV2\0Bump UV3\0Bump UV4\0Bump UV5\0Bump UV6\0Color "
                    "Channel 0\0Color Channel 1\0");
                AUTO_PROP(texGens[i].sourceParam,
                          static_cast<librii::gx::TexGenSrc>(src));
              }
              if (tm != nullptr &&
                  ImGui::CollapsingHeader("Texture Coordinate Generator",
                                          ImGuiTreeNodeFlags_DefaultOpen)) {
                // TODO: Effect matrix
                int xfmodel = static_cast<int>(tm->transformModel);
                bool is_j3d = dynamic_cast<const riistudio::j3d::Material*>(
                    &delegate.getActive());
                if (is_j3d) {
                  ImGui::Combo("Transform Model", &xfmodel,
                               "Basic (Not Recommended)\0"
                               "Maya\0");
                } else {
                  --xfmodel;
                  ImGui::Combo("Transform Model", &xfmodel,
                               "Maya\0"
                               "3DS Max\0"
                               "Softimage XSI\0");
                  ++xfmodel;
                }
                AUTO_PROP(
                    texMatrices[texmatrixid].transformModel,
                    static_cast<libcube::GCMaterialData::CommonTransformModel>(
                        xfmodel));
                // TODO: Not all backends support all modes..
                int mapMethod = 0;
                switch (tm->method) {
                default:
                case libcube::GCMaterialData::CommonMappingMethod::Standard:
                  mapMethod = 0;
                  break;
                case libcube::GCMaterialData::CommonMappingMethod::
                    EnvironmentMapping:
                  mapMethod = 1;
                  break;
                case libcube::GCMaterialData::CommonMappingMethod::
                    ViewProjectionMapping:
                  mapMethod = 2;
                  break;
                case libcube::GCMaterialData::CommonMappingMethod::
                    ProjectionMapping:
                  mapMethod = 3;
                  break;
                case libcube::GCMaterialData::CommonMappingMethod::
                    EnvironmentLightMapping:
                  mapMethod = 4;
                  break;
                case libcube::GCMaterialData::CommonMappingMethod::
                    EnvironmentSpecularMapping:
                  mapMethod = 5;
                  break;
                case libcube::GCMaterialData::CommonMappingMethod::
                    ManualEnvironmentMapping:
                  mapMethod = 6;
                  break;
                }
                ImGui::Combo("Mapping method", &mapMethod,
                             "Standard Mapping\0Environment Mapping\0View "
                             "Projection Mapping\0Manual Projection "
                             "Mapping\0Environment Light Mapping\0Environment "
                             "Specular Mapping\0Manual Environment Mapping\0");
                libcube::GCMaterialData::CommonMappingMethod newMapMethod =
                    libcube::GCMaterialData::CommonMappingMethod::Standard;
                using cmm = libcube::GCMaterialData::CommonMappingMethod;
                switch (mapMethod) {
                default:
                case 0:
                  newMapMethod = cmm::Standard;
                  break;
                case 1:
                  newMapMethod = cmm::EnvironmentMapping;
                  break;
                case 2:
                  newMapMethod = cmm::ViewProjectionMapping;
                  break;
                case 3:
                  newMapMethod = cmm::ProjectionMapping;
                  break;
                case 4:
                  newMapMethod = cmm::EnvironmentLightMapping;
                  break;
                case 5:
                  newMapMethod = cmm::EnvironmentSpecularMapping;
                  break;
                case 6:
                  newMapMethod = cmm::ManualEnvironmentMapping;
                  break;
                }
                AUTO_PROP(texMatrices[texmatrixid].method, newMapMethod);

                int mod = static_cast<int>(tm->option);
                ImGui::Combo(
                    "Option", &mod,
                    "Standard\0J3D Basic: Don't remap into texture space (Keep "
                    "-1..1 not 0...1)\0J3D Old: Keep translation column.");
                AUTO_PROP(
                    texMatrices[texmatrixid].option,
                    static_cast<libcube::GCMaterialData::CommonMappingOption>(
                        mod));
              }
              ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
          }
          if (tm == nullptr) {
            ImVec4 col{200.0f / 255.0f, 12.0f / 255.0f, 12.0f / 255.0f, 1.0f};
            ImGui::SetWindowFontScale(2.0f);
            ImGui::TextColored(col, "No texture matrix is attached");
            ImGui::SetWindowFontScale(1.0f);
          } else {
            ImGui::Text("(Texture Matrix %u)", (u32)texmatrixid);
          }
          if (tm != nullptr &&
              ImGui::CollapsingHeader("Transformation",
                                      ImGuiTreeNodeFlags_DefaultOpen)) {
            auto s = tm->scale;
            const auto rotate = glm::degrees(tm->rotate);
            auto r = rotate;
            auto t = tm->translate;
            ImGui::SliderFloat2("Scale", &s.x, 0.0f, 10.0f);
            ImGui::SliderFloat("Rotate", &r, 0.0f, 360.0f);
            ImGui::SliderFloat2("Translate", &t.x, -10.0f, 10.0f);
            AUTO_PROP(texMatrices[texmatrixid].scale, s);
            if (r != rotate)
              AUTO_PROP(texMatrices[texmatrixid].rotate, glm::radians(r));
            AUTO_PROP(texMatrices[texmatrixid].translate, t);

#ifdef BUILD_DEBUG
            const auto computed = glm::transpose(tm->compute({}, {}));
            Toolkit::Matrix44(computed);
#endif
          }
        }
        if (ImGui::CollapsingHeader("Tiling", ImGuiTreeNodeFlags_DefaultOpen)) {
          const char* wrap_modes = "Clamp\0Repeat\0Mirror\0";
          auto uTile = imcxx::Combo("U tiling", samp->mWrapU, wrap_modes);
          auto vTile = imcxx::Combo("V tiling", samp->mWrapV, wrap_modes);
          AUTO_PROP(samplers[i]->mWrapU, uTile);
          AUTO_PROP(samplers[i]->mWrapV, vTile);
        }
        if (ImGui::CollapsingHeader("Filtering",
                                    ImGuiTreeNodeFlags_DefaultOpen)) {
          int magBase = static_cast<int>(samp->mMagFilter);

          auto min_filt = librii::hx::elevateTextureFilter(samp->mMinFilter);

          const char* linNear = "Nearest (no interpolation/pixelated)\0Linear "
                                "(interpolated/blurry)\0";

          ImGui::Combo("Interpolation when scaled up", &magBase, linNear);
          AUTO_PROP(samplers[i]->mMagFilter,
                    static_cast<librii::gx::TextureFilter>(magBase));
          ImGui::Combo("Interpolation when scaled down", &min_filt.minBase,
                       linNear);

          ImGui::Checkbox("Use mipmap", &min_filt.mip);

          {
            ConditionalActive g(min_filt.mip);

            if (ImGui::CollapsingHeader("Mipmapping",
                                        ImGuiTreeNodeFlags_DefaultOpen)) {
              ImGui::Combo("Interpolation type", &min_filt.minMipBase, linNear);

              float lodbias = samp->mLodBias;
              ImGui::SliderFloat("LOD bias", &lodbias, -4.0f, 3.99f);
              AUTO_PROP(samplers[i]->mLodBias, lodbias);

              bool edgelod = samp->bEdgeLod;
              ImGui::Checkbox("Edge LOD", &edgelod);
              AUTO_PROP(samplers[i]->bEdgeLod, edgelod);

              {
                ConditionalActive g(edgelod);

                bool mipBiasClamp = samp->bBiasClamp;
                ImGui::Checkbox("Bias clamp", &mipBiasClamp);
                AUTO_PROP(samplers[i]->bBiasClamp, mipBiasClamp);

                int maxaniso = 0;
                switch (samp->mMaxAniso) {
                case librii::gx::AnisotropyLevel::x1:
                  maxaniso = 1;
                  break;
                case librii::gx::AnisotropyLevel::x2:
                  maxaniso = 2;
                  break;
                case librii::gx::AnisotropyLevel::x4:
                  maxaniso = 4;
                  break;
                }
                ImGui::SliderInt("Anisotropic filtering level", &maxaniso, 1,
                                 4);
                librii::gx::AnisotropyLevel alvl =
                    librii::gx::AnisotropyLevel::x1;
                switch (maxaniso) {
                case 1:
                  alvl = librii::gx::AnisotropyLevel::x1;
                  break;
                case 2:
                  alvl = librii::gx::AnisotropyLevel::x2;
                  break;
                case 3:
                case 4:
                  alvl = librii::gx::AnisotropyLevel::x4;
                  break;
                }
                AUTO_PROP(samplers[i]->mMaxAniso, alvl);
              }
            }
          }

          librii::gx::TextureFilter computedMin =
              librii::hx::lowerTextureFilter(min_filt);

          AUTO_PROP(samplers[i]->mMinFilter, computedMin);
        }
        ImGui::EndTabItem();
      }
    }

    for (std::size_t i = 0; i < matData.texGens.size(); ++i) {
      if (!open[i]) {
        deleteSampler(matData, i);
        delegate.commit("Erased a sampler");
        break;
      }
    }
    ImGui::EndTabBar();
  }
}

kpi::RegisterPropertyView<IGCMaterial, SamplerSurface> SamplerSurfaceInstaller;

} // namespace libcube::UI
