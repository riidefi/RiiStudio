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
    printf("[Warning] Cannot add sampler on material %s\n"_j, d.name.c_str());
    return;
  }

  gx::TexCoordGen gen;
  gen.setMatrixIndex(d.texMatrices.size());
  d.texGens.push_back(gen);
  d.texMatrices.push_back(GCMaterialData::TexMatrix{});
  d.samplers.push_back(GCMaterialData::SamplerData{});
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
  static inline const char* name() { return "Samplers"_j; }
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
    ImGui::TextUnformatted("No valid image."_j);
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

librii::gx::AnisotropyLevel DrawAniso(librii::gx::AnisotropyLevel alvl) {
  int maxaniso = 0;
  switch (alvl) {
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

  ImGui::SliderInt("Anisotropic filtering level"_j, &maxaniso, 1, 4);

  switch (maxaniso) {
  default:
  case 1:
    return librii::gx::AnisotropyLevel::x1;
  case 2:
    return librii::gx::AnisotropyLevel::x2;
  case 3:
  case 4:
    return librii::gx::AnisotropyLevel::x4;
  }
}

librii::gx::TexGenSrc DrawTGSource(librii::gx::TexGenSrc source) {
  return imcxx::Combo("Source data"_j, source,
                      "Position\0"
                      "Normal\0"
                      "Binormal\0"
                      "Tangent\0"
                      "UV 0\0"
                      "UV 1\0"
                      "UV 2\0"
                      "UV 3\0"
                      "UV 4\0"
                      "UV 5\0"
                      "UV 6\0"
                      "UV 7\0"
                      "Bump UV0\0"
                      "Bump UV1\0"
                      "Bump UV2\0"
                      "Bump UV3\0"
                      "Bump UV4\0"
                      "Bump UV5\0"
                      "Bump UV6\0"
                      "Color Channel 0\0"
                      "Color Channel 1\0"_j);
}

librii::hx::BaseTexGenFunction
DrawBaseTGFunc(librii::hx::BaseTexGenFunction func) {
  return imcxx::Combo("Function"_j, func,
                      "Standard Texture Matrix\0"
                      "Bump Mapping: Use vertex lighting calculation result.\0"
                      "SRTG: Map R(ed) and G(reen) components of a color "
                      "channel to U/V coordinates\0"_j);
}

libcube::GCMaterialData::CommonTransformModel
DrawCommXModel(libcube::GCMaterialData::CommonTransformModel mdl, bool is_j3d) {
  int xfmodel = static_cast<int>(mdl);
  if (is_j3d) {
    ImGui::Combo("Transform Model"_j, &xfmodel,
                 "Basic (Not Recommended)\0"
                 "Maya\0"_j);
  } else {
    --xfmodel;
    ImGui::Combo("Transform Model"_j, &xfmodel,
                 "Maya\0"
                 "3DS Max\0"
                 "Softimage XSI\0"_j);
    ++xfmodel;
  }

  return static_cast<libcube::GCMaterialData::CommonTransformModel>(xfmodel);
}

auto DrawCommMapMethod(libcube::GCMaterialData::CommonMappingMethod method) {
  int mapMethod = 0;
  switch (method) {
  default:
  case libcube::GCMaterialData::CommonMappingMethod::Standard:
    mapMethod = 0;
    break;
  case libcube::GCMaterialData::CommonMappingMethod::EnvironmentMapping:
    mapMethod = 1;
    break;
  case libcube::GCMaterialData::CommonMappingMethod::ViewProjectionMapping:
    mapMethod = 2;
    break;
  case libcube::GCMaterialData::CommonMappingMethod::ProjectionMapping:
    mapMethod = 3;
    break;
  case libcube::GCMaterialData::CommonMappingMethod::EnvironmentLightMapping:
    mapMethod = 4;
    break;
  case libcube::GCMaterialData::CommonMappingMethod::EnvironmentSpecularMapping:
    mapMethod = 5;
    break;
  case libcube::GCMaterialData::CommonMappingMethod::ManualEnvironmentMapping:
    mapMethod = 6;
    break;
  }
  ImGui::Combo("Mapping method"_j, &mapMethod,
               "Standard Mapping\0"
               "Environment Mapping\0"
               "View Projection Mapping\0"
               "Manual Projection Mapping\0"
               "Environment Light Mapping\0"
               "Environment Specular Mapping\0"
               "Manual Environment Mapping\0"_j);

  using cmm = libcube::GCMaterialData::CommonMappingMethod;
  switch (mapMethod) {
  default:
  case 0:
    return cmm::Standard;
  case 1:
    return cmm::EnvironmentMapping;
  case 2:
    return cmm::ViewProjectionMapping;
  case 3:
    return cmm::ProjectionMapping;
  case 4:
    return cmm::EnvironmentLightMapping;
  case 5:
    return cmm::EnvironmentSpecularMapping;
  case 6:
    return cmm::ManualEnvironmentMapping;
  }
}

auto DrawCommMapMod(libcube::GCMaterialData::CommonMappingOption mod) {
  return imcxx::Combo(
      "Option"_j, mod,
      "Standard\0"
      "J3D Basic: Don't remap into texture space (Keep -1..1 not 0...1)\0"
      "J3D Old: Keep translation column.\0"_j);
}

void drawProperty(kpi::PropertyDelegate<IGCMaterial>& delegate,
                  SamplerSurface& surface) {
  auto& matData = delegate.getActive().getMaterialData();

  if (matData.texGens.size() != matData.samplers.size()) {
    ImGui::TextUnformatted("Cannot edit: source data is invalid!"_j);
    return;
  }

  if (ImGui::Button("Add Sampler"_j)) {
    addSampler(delegate);
  }

  if (ImGui::BeginTabBar("Textures"_j)) {
    llvm::SmallVector<bool, 16> open(matData.texGens.size(), true);
    for (std::size_t i = 0; i < matData.texGens.size(); ++i) {
      auto& tg = matData.texGens[i];

      bool identitymatrix = tg.isIdentityMatrix();
      int texmatrixid = tg.getMatrixIndex();

      GCMaterialData::TexMatrix* tm = nullptr;
      if (tg.matrix != gx::TexMatrix::Identity)
        tm = &matData.texMatrices[texmatrixid]; // TODO: Proper lookup
      auto* samp = &matData.samplers[i];
      const libcube::Scene* pScn = dynamic_cast<const libcube::Scene*>(
          dynamic_cast<const kpi::IObject*>(&delegate.getActive())
              ->childOf->childOf);
      const auto mImgs = delegate.getActive().getTextureSource(*pScn);
      if (ImGui::BeginTabItem((std::string("Texture "_j) + std::to_string(i) +
                               " [" + samp->mTexture + "]")
                                  .c_str(),
                              &open[i])) {
        if (ImGui::CollapsingHeader("Image"_j,
                                    ImGuiTreeNodeFlags_DefaultOpen)) {
          if (auto result = TextureImageCombo(samp->mTexture.c_str(), mImgs,
                                              delegate.mEd);
              !result.empty()) {
            AUTO_PROP(samplers[i].mTexture, result);
          }

          if (samp != nullptr)
            drawSamplerImage(mImgs, *samp, surface);
        }
        if (ImGui::CollapsingHeader("Mapping"_j,
                                    ImGuiTreeNodeFlags_DefaultOpen)) {
          if (ImGui::BeginTabBar("Mapping"_j)) {
            if (ImGui::BeginTabItem("Standard"_j)) {
              ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Advanced"_j)) {
              if (ImGui::CollapsingHeader("Texture Coordinate Generator"_j,
                                          ImGuiTreeNodeFlags_DefaultOpen)) {
                librii::hx::TexGenType hx_tg =
                    librii::hx::elevateTexGenType(tg.func);

                hx_tg.basefunc = DrawBaseTGFunc(hx_tg.basefunc);
                {
                  ConditionalActive g(
                      hx_tg.basefunc ==
                      librii::hx::BaseTexGenFunction::TextureMatrix);
                  ImGui::Combo("Matrix Size"_j, &hx_tg.mtxtype,
                               "UV Matrix: 2x4\0"
                               "UVW Matrix: 3x4\0"_j);

                  ImGui::Checkbox("Identity Matrix"_j, &identitymatrix);
                  ImGui::SameLine();
                  {
                    ConditionalActive g2(!identitymatrix);
                    ImGui::SliderInt("Matrix ID"_j, &texmatrixid, 0, 7);
                  }

                  auto actual_matrix = static_cast<librii::gx::TexMatrix>(
                      static_cast<int>(librii::gx::TexMatrix::TexMatrix0) +
                      std::max(0, texmatrixid) * 3);

                  librii::gx::TexMatrix newtexmatrix =
                      identitymatrix ? librii::gx::TexMatrix::Identity
                                     : actual_matrix;
                  AUTO_PROP(texGens[i].matrix, newtexmatrix);
                }
                {
                  ConditionalActive g(hx_tg.basefunc ==
                                      librii::hx::BaseTexGenFunction::Bump);
                  ImGui::SliderInt("Hardware light ID"_j, &hx_tg.lightid, 0, 7);
                }

                librii::gx::TexGenType newfunc =
                    librii::hx::lowerTexGenType(hx_tg);
                AUTO_PROP(texGens[i].func, newfunc);

                auto src = DrawTGSource(tg.sourceParam);
                AUTO_PROP(texGens[i].sourceParam, src);
              }
              if (tm != nullptr &&
                  ImGui::CollapsingHeader("Texture Coordinate Generator"_j,
                                          ImGuiTreeNodeFlags_DefaultOpen)) {
                // TODO: Effect matrix
                {
                  bool is_j3d = dynamic_cast<const riistudio::j3d::Material*>(
                      &delegate.getActive());

                  auto xfmodel = DrawCommXModel(tm->transformModel, is_j3d);
                  AUTO_PROP(texMatrices[texmatrixid].transformModel, xfmodel);
                }
                {
                  // TODO: Not all backends support all modes..
                  auto newMapMethod = DrawCommMapMethod(tm->method);
                  AUTO_PROP(texMatrices[texmatrixid].method, newMapMethod);
                }
                {
                  auto mod = DrawCommMapMod(tm->option);
                  AUTO_PROP(texMatrices[texmatrixid].option, mod);
                }
              }
              ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
          }
          if (tm == nullptr) {
            ImVec4 col{200.0f / 255.0f, 12.0f / 255.0f, 12.0f / 255.0f, 1.0f};
            ImGui::SetWindowFontScale(2.0f);
            ImGui::TextColored(col, "%s", "No texture matrix is attached"_j);
            ImGui::SetWindowFontScale(1.0f);
          } else {
            ImGui::Text("(Texture Matrix %u)"_j, (u32)texmatrixid);
          }
          if (tm != nullptr &&
              ImGui::CollapsingHeader("Transformation"_j,
                                      ImGuiTreeNodeFlags_DefaultOpen)) {
            auto s = tm->scale;
            const auto rotate = glm::degrees(tm->rotate);
            auto r = rotate;
            auto t = tm->translate;
            ImGui::SliderFloat2("Scale"_j, &s.x, 0.0f, 10.0f);
            ImGui::SliderFloat("Rotate"_j, &r, 0.0f, 360.0f);
            ImGui::SliderFloat2("Translate"_j, &t.x, -10.0f, 10.0f);
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
        if (ImGui::CollapsingHeader("Tiling"_j,
                                    ImGuiTreeNodeFlags_DefaultOpen)) {
          const char* wrap_modes = "Clamp\0"
                                   "Repeat\0"
                                   "Mirror\0"_j;
          auto uTile = imcxx::Combo("U tiling"_j, samp->mWrapU, wrap_modes);
          auto vTile = imcxx::Combo("V tiling"_j, samp->mWrapV, wrap_modes);
          AUTO_PROP(samplers[i].mWrapU, uTile);
          AUTO_PROP(samplers[i].mWrapV, vTile);
        }
        if (ImGui::CollapsingHeader("Filtering"_j,
                                    ImGuiTreeNodeFlags_DefaultOpen)) {
          auto magBase = samp->mMagFilter;

          auto min_filt = librii::hx::elevateTextureFilter(samp->mMinFilter);

          const char* linNear = "Nearest (no interpolation/pixelated)\0"
                                "Linear (interpolated/blurry)\0"_j;

          magBase =
              imcxx::Combo("Interpolation when scaled up"_j, magBase, linNear);
          AUTO_PROP(samplers[i].mMagFilter, magBase);
          ImGui::Combo("Interpolation when scaled down"_j, &min_filt.minBase,
                       linNear);

          ImGui::Checkbox("Use mipmap"_j, &min_filt.mip);

          {
            ConditionalActive g(min_filt.mip);

            if (ImGui::CollapsingHeader("Mipmapping"_j,
                                        ImGuiTreeNodeFlags_DefaultOpen)) {
              ImGui::Combo("Interpolation type"_j, &min_filt.minMipBase,
                           linNear);

              float lodbias = samp->mLodBias;
              ImGui::SliderFloat("LOD bias"_j, &lodbias, -4.0f, 3.99f);
              AUTO_PROP(samplers[i].mLodBias, lodbias);

              bool edgelod = samp->bEdgeLod;
              ImGui::Checkbox("Edge LOD"_j, &edgelod);
              AUTO_PROP(samplers[i].bEdgeLod, edgelod);

              {
                ConditionalActive g(edgelod);

                bool mipBiasClamp = samp->bBiasClamp;
                ImGui::Checkbox("Bias clamp"_j, &mipBiasClamp);
                AUTO_PROP(samplers[i].bBiasClamp, mipBiasClamp);

                auto alvl = DrawAniso(samp->mMaxAniso);
                AUTO_PROP(samplers[i].mMaxAniso, alvl);
              }
            }
          }

          librii::gx::TextureFilter computedMin =
              librii::hx::lowerTextureFilter(min_filt);

          AUTO_PROP(samplers[i].mMinFilter, computedMin);
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
