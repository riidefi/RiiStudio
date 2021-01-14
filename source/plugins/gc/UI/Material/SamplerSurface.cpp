#include "Common.hpp"
#include <core/3d/ui/Image.hpp> // for ImagePreview
#include <plugins/gc/Export/Scene.hpp>

#undef near
#undef far
namespace libcube::UI {

using namespace riistudio::util;

struct SamplerSurface final {
  static inline const char* name = "Samplers";
  static inline const char* icon = (const char*)ICON_FA_IMAGES;

  // Mark this surface to be more than an IDL tag.
  int tag_stateful;

  riistudio::frontend::ImagePreview mImg; // In mat sampler
  std::string mLastImg;
};

void drawProperty(kpi::PropertyDelegate<IGCMaterial>& delegate,
                  SamplerSurface& surface) {
  auto& matData = delegate.getActive().getMaterialData();

  if (matData.texGens.size() != matData.samplers.size()) {
    ImGui::Text("Cannot edit: source data is invalid!");
    return;
  }

  if (ImGui::Button("Add Sampler")) {
    for (auto* obj : delegate.mAffected) {
      auto& d = obj->getMaterialData();

      if (d.texGens.size() != d.samplers.size()) {
        printf("[Warning] Cannot add sampler on material %s\n", d.name.c_str());
      }

      gx::TexCoordGen gen;
      gen.setMatrixIndex(d.texMatrices.size());
      d.texGens.push_back(gen);
      d.texMatrices.push_back(GCMaterialData::TexMatrix{});
      d.samplers.push_back(std::make_unique<GCMaterialData::SamplerData>());
    }
    delegate.commit("Added sampler");
  }

  if (ImGui::BeginTabBar("Textures")) {
    llvm::SmallVector<bool, 16> open(matData.texGens.nElements, true);
    for (std::size_t i = 0; i < matData.texGens.nElements; ++i) {
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

          const riistudio::lib3d::Texture* curImg = nullptr;

          for (auto& it : mImgs) {
            if (it.getName() == samp->mTexture) {
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
        if (ImGui::CollapsingHeader("Mapping",
                                    ImGuiTreeNodeFlags_DefaultOpen)) {
          if (ImGui::BeginTabBar("Mapping")) {
            if (ImGui::BeginTabItem("Standard")) {
              ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Advanced")) {
              if (ImGui::CollapsingHeader("Texture Coordinate Generator",
                                          ImGuiTreeNodeFlags_DefaultOpen)) {
                int basefunc = 0;
                int mtxtype = 0;
                int lightid = 0;

                switch (tg.func) {
                case librii::gx::TexGenType::Matrix2x4:
                  basefunc = 0;
                  mtxtype = 0;
                  break;
                case librii::gx::TexGenType::Matrix3x4:
                  basefunc = 0;
                  mtxtype = 1;
                  break;
                case librii::gx::TexGenType::Bump0:
                case librii::gx::TexGenType::Bump1:
                case librii::gx::TexGenType::Bump2:
                case librii::gx::TexGenType::Bump3:
                case librii::gx::TexGenType::Bump4:
                case librii::gx::TexGenType::Bump5:
                case librii::gx::TexGenType::Bump6:
                case librii::gx::TexGenType::Bump7:
                  basefunc = 1;
                  lightid = static_cast<int>(tg.func) -
                            static_cast<int>(librii::gx::TexGenType::Bump0);
                  break;
                case librii::gx::TexGenType::SRTG:
                  basefunc = 2;
                  break;
                }

                ImGui::Combo(
                    "Function", &basefunc,
                    "Standard Texture Matrix\0Bump Mapping: Use vertex "
                    "lighting calculation result.\0SRTG: Map R(ed) and G(reen) "
                    "components of a color channel to U/V coordinates\0");
                {
                  ConditionalActive g(basefunc == 0);
                  ImGui::Combo("Matrix Size", &mtxtype,
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
                  ConditionalActive g(basefunc == 1);
                  ImGui::SliderInt("Hardware light ID", &lightid, 0, 7);
                }

                librii::gx::TexGenType newfunc =
                    librii::gx::TexGenType::Matrix2x4;
                switch (basefunc) {
                case 0:
                  newfunc = mtxtype ? librii::gx::TexGenType::Matrix3x4
                                    : librii::gx::TexGenType::Matrix2x4;
                  break;
                case 1:
                  newfunc = static_cast<librii::gx::TexGenType>(
                      static_cast<int>(librii::gx::TexGenType::Bump0) +
                      lightid);
                  break;
                case 2:
                  newfunc = librii::gx::TexGenType::SRTG;
                  break;
                }
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
                ImGui::Combo("Transform Model", &xfmodel,
                             " Default\0 Maya\0 3DS Max\0 Softimage XSI\0");
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
          int sTile = static_cast<int>(samp->mWrapU);
          ImGui::Combo("U tiling", &sTile, "Clamp\0Repeat\0Mirror\0");
          int tTile = static_cast<int>(samp->mWrapV);
          ImGui::Combo("V tiling", &tTile, "Clamp\0Repeat\0Mirror\0");
          AUTO_PROP(samplers[i]->mWrapU,
                    static_cast<librii::gx::TextureWrapMode>(sTile));
          AUTO_PROP(samplers[i]->mWrapV,
                    static_cast<librii::gx::TextureWrapMode>(tTile));
        }
        if (ImGui::CollapsingHeader("Filtering",
                                    ImGuiTreeNodeFlags_DefaultOpen)) {
          int magBase = static_cast<int>(samp->mMagFilter);

          int minBase = 0;
          int minMipBase = 0;
          bool mip = false;

          switch (samp->mMinFilter) {
          case librii::gx::TextureFilter::near_mip_near:
            mip = true;
          case librii::gx::TextureFilter::near:
            minBase = minMipBase =
                static_cast<int>(librii::gx::TextureFilter::near);
            break;
          case librii::gx::TextureFilter::lin_mip_lin:
            mip = true;
          case librii::gx::TextureFilter::linear:
            minBase = minMipBase =
                static_cast<int>(librii::gx::TextureFilter::linear);
            break;
          case librii::gx::TextureFilter::near_mip_lin:
            mip = true;
            minBase = static_cast<int>(librii::gx::TextureFilter::near);
            minMipBase = static_cast<int>(librii::gx::TextureFilter::linear);
            break;
          case librii::gx::TextureFilter::lin_mip_near:
            mip = true;
            minBase = static_cast<int>(librii::gx::TextureFilter::linear);
            minMipBase = static_cast<int>(librii::gx::TextureFilter::near);
            break;
          }

          const char* linNear = "Nearest (no interpolation/pixelated)\0Linear "
                                "(interpolated/blurry)\0";

          ImGui::Combo("Interpolation when scaled up", &magBase, linNear);
          AUTO_PROP(samplers[i]->mMagFilter,
                    static_cast<librii::gx::TextureFilter>(magBase));
          ImGui::Combo("Interpolation when scaled down", &minBase, linNear);

          ImGui::Checkbox("Use mipmap", &mip);

          {
            ConditionalActive g(mip);

            if (ImGui::CollapsingHeader("Mipmapping",
                                        ImGuiTreeNodeFlags_DefaultOpen)) {
              ImGui::Combo("Interpolation type", &minMipBase, linNear);

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
              librii::gx::TextureFilter::near;
          if (!mip) {
            computedMin = static_cast<librii::gx::TextureFilter>(minBase);
          } else {
            bool baseLin = static_cast<librii::gx::TextureFilter>(minBase) ==
                           librii::gx::TextureFilter::linear;
            if (static_cast<librii::gx::TextureFilter>(minMipBase) ==
                librii::gx::TextureFilter::linear) {
              computedMin = baseLin ? librii::gx::TextureFilter::lin_mip_lin
                                    : librii::gx::TextureFilter::near_mip_lin;
            } else {
              computedMin = baseLin ? librii::gx::TextureFilter::lin_mip_near
                                    : librii::gx::TextureFilter::near_mip_near;
            }
          }

          AUTO_PROP(samplers[i]->mMinFilter, computedMin);
        }
        ImGui::EndTabItem();
      }
    }
    bool changed = false;
    for (std::size_t i = 0; i < matData.texGens.nElements; ++i) {
      if (open[i])
        continue;

      changed = true;

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

      delegate.commit("Erased a sampler");

      break;
    }
    ImGui::EndTabBar();
  }
}

kpi::RegisterPropertyView<IGCMaterial, SamplerSurface> SamplerSurfaceInstaller;

} // namespace libcube::UI
