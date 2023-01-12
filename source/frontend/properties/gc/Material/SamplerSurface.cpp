#include "Common.hpp"
#include <frontend/widgets/Lib3dImage.hpp> // for Lib3dCachedImagePreview
#include <imcxx/Widgets.hpp>
#include <librii/hx/TexGenType.hpp>
#include <librii/hx/TextureFilter.hpp>
#include <plugins/gc/Export/Scene.hpp>
#include <plugins/j3d/Material.hpp>
#include <rsl/FsDialog.hpp>
#include <rsl/SmallVector.hpp>

namespace libcube::UI {

[[nodiscard]] Result<void> addSampler(libcube::GCMaterialData& d) {
  if (d.texGens.size() != d.samplers.size() || d.texGens.size() == 10 ||
      d.samplers.size() == 8 || d.texMatrices.size() == 10) {
    return std::unexpected(
        std::format("[Warning] Cannot add sampler on material {}\n", d.name));
  }

  gx::TexCoordGen gen;
  gen.setMatrixIndex(d.texMatrices.size());
  d.texGens.push_back(gen);
  d.texMatrices.push_back(GCMaterialData::TexMatrix{});
  d.samplers.push_back(GCMaterialData::SamplerData{});
  return {};
}

[[nodiscard]] Result<void> deleteSampler(libcube::GCMaterialData& matData,
                                         size_t i) {
  // Only one may be deleted at a time
  matData.samplers.erase(i);
  EXPECT(matData.texGens[i].getMatrixIndex() == i);
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
  return {};
}

[[nodiscard]] Result<void>
addSampler(kpi::PropertyDelegate<libcube::IGCMaterial>& delegate) {
  std::vector<Result<void>> results;
  for (auto* obj : delegate.mAffected) {
    results.push_back(addSampler(obj->getMaterialData()));
  }
  std::string errors;
  for (auto& result : results) {
    if (!result) {
      errors += result.error() + "\n";
    }
  }
  delegate.commit("Added sampler");
  if (errors.size()) {
    return std::unexpected(errors);
  }
  return {};
}

using namespace riistudio::util;

struct SamplerSurface final {
  static inline const char* name() { return "Samplers"_j; }
  static inline const char* icon = (const char*)ICON_FA_IMAGES;

  // Mark this surface to be more than an IDL tag.
  int tag_stateful;

  riistudio::frontend::Lib3dCachedImagePreview mImg; // In mat sampler
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
    const auto aspect_ratio = static_cast<f32>(curImg->getWidth()) /
                              static_cast<f32>(curImg->getHeight());
    surface.mImg.draw(*curImg, 128.0f * aspect_ratio, 128.0f);
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
struct Mapping {
  struct UV {
    u8 channel = 0;
    bool operator==(const UV&) const = default;
  };
  struct Env {
    bool operator==(const Env&) const = default;
  };
  struct LightEnv {
    s8 lightId = -1;
    bool operator==(const LightEnv&) const = default;
  };
  struct SpecEnv {
    s8 lightId = -1;
    bool operator==(const SpecEnv&) const = default;
  };
  struct Project {
    s8 camId = -1;
    bool operator==(const Project&) const = default;
  };
  struct Advanced {
    bool operator==(const Advanced&) const = default;
  };
};
using Mappings =
    std::variant<Mapping::UV, Mapping::Env, Mapping::LightEnv, Mapping::SpecEnv,
                 Mapping::Project, Mapping::Advanced>;

Mappings DrawMappings(Mappings map) {
  int id = 0;
  int uv = 0;
  int lightId = -1;
  int camId = -1;
  if (auto* x = std::get_if<Mapping::UV>(&map)) {
    id = 0;
    uv = x->channel;
  } else if (auto* x = std::get_if<Mapping::Env>(&map)) {
    id = 1;
  } else if (auto* x = std::get_if<Mapping::LightEnv>(&map)) {
    id = 2;
    lightId = x->lightId;
  } else if (auto* x = std::get_if<Mapping::SpecEnv>(&map)) {
    id = 3;
    lightId = x->lightId;
  } else if (auto* x = std::get_if<Mapping::Project>(&map)) {
    id = 4;
    camId = x->camId;
  } else if (auto* x = std::get_if<Mapping::Advanced>(&map)) {
    id = 5;
  }
  ImGui::Combo("Method", &id,
               "UV Mapping\0"
               "Environment Mapping\0"
               "Light Environment Mapping\0"
               "Specular Environment Mapping\0"
               "Projection Mapping\0"
               "Advanced (...)\0"_j);
  switch (id) {
  case 0:
    ImGui::InputInt("UV Channel"_j, &uv);
    break;
  case 2:
  case 3:
    ImGui::InputInt("Light ID"_j, &lightId);
    break;
  case 4:
    ImGui::InputInt("Camera ID"_j, &camId);
    break;
  }
  switch (id) {
  case 1:
  case 2:
  case 3: {
    // Source: Wikipedia
    ImGui::TextWrapped(
        "%s",
        "Sphere mapping (or spherical environment mapping) is a type of "
        "reflection mapping that approximates reflective surfaces by "
        "considering the environment to be an infinitely far-away spherical "
        "wall. This environment is stored as a texture depicting what a "
        "mirrored sphere would look like if it were placed into the "
        "environment, using an orthographic projection (as opposed to one with "
        "perspective). This texture contains reflective data for the entire "
        "environment, except for the spot directly behind the sphere. (For one "
        "example of such an object, see Escher's drawing Hand with Reflecting "
        "Sphere.)"_j);
    [&]() {
      switch (id) {
      case 1:
        ImGui::TextWrapped(
            "%s",
            "This setting models a perfectly reflective surface. A perfectly "
            "reflective reflects all incoming light with perfect accuracy, "
            "without absorbing any of it. This would create a mirror-like "
            "appearance, where the surface appears to be a perfect reflection "
            "of its surroundings. Perfectly reflective materials are not found "
            "in nature, but they can be simulated using computer graphics and "
            "special coatings."_j);
        break;
      case 2:
        ImGui::TextWrapped(
            "%s",
            "This setting models a perfectly diffuse surface affected only by "
            "the light specified above. A diffuse surface scatters light "
            "evenly in all directions. When light hits a diffuse surface, it "
            "is absorbed and then reflected in many directions, giving the "
            "surface a matte or non-shiny appearance. Common examples of "
            "diffuse materials include paper, cloth, and most types of paint."_j);
        break;
      case 3:
        ImGui::TextWrapped(
            "%s",
            "This setting models a perfectly diffuse surface affected only by "
            "the light specified above. A specular surface reflects light in a "
            "concentrated way, creating a shiny or glossy appearance. When "
            "light hits a specular surface, it is reflected in a single "
            "direction, creating a highlight or specular reflection. Examples "
            "of specular materials include metal, glass, and most types of "
            "plastic."_j);
        break;
      }
    }();
    break;
  }
  case 4:
    ImGui::TextWrapped(
        "%s", "Draws a 2D image on a 3D surface from the camera's "
              "POV. Appears as if there is a projector behind the "
              "camera shining the image onto objects with this material."_j);
    break;
  }

  switch (id) {
  case 0:
    return Mapping::UV{.channel = static_cast<u8>(uv)};
  case 1:
    return Mapping::Env{};
  case 2:
    return Mapping::LightEnv{.lightId = static_cast<s8>(lightId)};
  case 3:
    return Mapping::SpecEnv{.lightId = static_cast<s8>(lightId)};
  case 4:
    return Mapping::Project{.camId = static_cast<s8>(camId)};
  case 5:
  default:
    return Mapping::Advanced{};
  }
}

Mappings Mappings_from(const libcube::GCMaterialData::TexMatrix& m,
                       const librii::gx::TexCoordGen& g) {
  using cmm = libcube::GCMaterialData::CommonMappingMethod;
  switch (m.method) {
  case cmm::Standard: {
    if (g.func != librii::gx::TexGenType::Matrix2x4) {
      break;
    }
    using sp = librii::gx::TexGenSrc;
    switch (g.sourceParam) {
    case sp::UV0:
      return Mapping::UV{.channel = 0};
    case sp::UV1:
      return Mapping::UV{.channel = 1};
    case sp::UV2:
      return Mapping::UV{.channel = 2};
    case sp::UV3:
      return Mapping::UV{.channel = 3};
    case sp::UV4:
      return Mapping::UV{.channel = 4};
    case sp::UV5:
      return Mapping::UV{.channel = 5};
    case sp::UV6:
      return Mapping::UV{.channel = 6};
    case sp::UV7:
      return Mapping::UV{.channel = 7};
    default:
      break;
    }
  }
  case cmm::EnvironmentMapping: {
    if (g.func != librii::gx::TexGenType::Matrix3x4) {
      break;
    }
    if (g.sourceParam != librii::gx::TexGenSrc::Normal) {
      break;
    }
    return Mapping::Env{};
  }
  case cmm::EnvironmentLightMapping: {
    if (g.func != librii::gx::TexGenType::Matrix3x4) {
      break;
    }
    if (g.sourceParam != librii::gx::TexGenSrc::Normal) {
      break;
    }
    return Mapping::LightEnv{.lightId = m.lightIdx};
  }
  case cmm::EnvironmentSpecularMapping: {
    if (g.func != librii::gx::TexGenType::Matrix3x4) {
      break;
    }
    if (g.sourceParam != librii::gx::TexGenSrc::Normal) {
      break;
    }
    return Mapping::SpecEnv{.lightId = m.lightIdx};
  }
  case cmm::ViewProjectionMapping: {
    if (g.func != librii::gx::TexGenType::Matrix3x4) {
      break;
    }
    if (g.sourceParam != librii::gx::TexGenSrc::Position) {
      break;
    }
    return Mapping::Project{.camId = m.camIdx};
  }
  default:
    break;
  }

  return Mapping::Advanced{};
}
void Mappings_to(Mappings map, libcube::GCMaterialData::TexMatrix& m,
                 librii::gx::TexCoordGen& g) {
  using cmm = libcube::GCMaterialData::CommonMappingMethod;
  using sp = librii::gx::TexGenSrc;

  if (auto* x = std::get_if<Mapping::UV>(&map)) {
    m.method = cmm::Standard;
    g.func = librii::gx::TexGenType::Matrix2x4;
    switch (x->channel) {
    case 0:
      g.sourceParam = sp::UV0;
      break;
    case 1:
      g.sourceParam = sp::UV1;
      break;
    case 2:
      g.sourceParam = sp::UV2;
      break;
    case 3:
      g.sourceParam = sp::UV3;
      break;
    case 4:
      g.sourceParam = sp::UV4;
      break;
    case 5:
      g.sourceParam = sp::UV5;
      break;
    case 6:
      g.sourceParam = sp::UV6;
      break;
    case 7:
      g.sourceParam = sp::UV7;
      break;
    default:
      break;
    }
  } else if (auto* x = std::get_if<Mapping::Env>(&map)) {
    m.method = cmm::EnvironmentMapping;
    g.func = librii::gx::TexGenType::Matrix3x4;
    g.sourceParam = librii::gx::TexGenSrc::Normal;
  } else if (auto* x = std::get_if<Mapping::LightEnv>(&map)) {
    m.method = cmm::EnvironmentLightMapping;
    g.func = librii::gx::TexGenType::Matrix3x4;
    g.sourceParam = librii::gx::TexGenSrc::Normal;
    m.lightIdx = x->lightId;
  } else if (auto* x = std::get_if<Mapping::SpecEnv>(&map)) {
    m.method = cmm::EnvironmentSpecularMapping;
    g.func = librii::gx::TexGenType::Matrix3x4;
    g.sourceParam = librii::gx::TexGenSrc::Normal;
    m.lightIdx = x->lightId;
  } else if (auto* x = std::get_if<Mapping::Project>(&map)) {
    m.method = cmm::ViewProjectionMapping;
    g.func = librii::gx::TexGenType::Matrix3x4;
    g.sourceParam = librii::gx::TexGenSrc::Position;
    m.camIdx = x->camId;
  }
}

void drawProperty(kpi::PropertyDelegate<IGCMaterial>& delegate,
                  SamplerSurface& surface) {
  auto& matData = delegate.getActive().getMaterialData();
  const bool is_j3d =
      dynamic_cast<const riistudio::j3d::Material*>(&delegate.getActive());

  if (matData.texGens.size() != matData.samplers.size()) {
    ImGui::TextUnformatted("Cannot edit: source data is invalid!"_j);
    return;
  }

  if (ImGui::Button("Add Sampler"_j)) {
    auto ok = addSampler(delegate);
    if (!ok) {
      rsl::ErrorDialog("Could not add sampler to some selected materials:\n"_j +
                       ok.error());
    }
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
      if (ImGui::BeginTabItem(
              std::format("Sampler {} [{}]###<internal_id=texture_{}>", i,
                          samp->mTexture, i)
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
        if (ImGui::BeginTabBar("Mapping"_j)) {
          if (ImGui::BeginTabItem("Standard"_j)) {
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
              const auto computed =
                  glm::transpose(tm->compute({}, {}).value_or(glm::mat4{0.0f}));
              Toolkit::Matrix44(computed);
#endif
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

              auto min_filt =
                  librii::hx::elevateTextureFilter(samp->mMinFilter);

              const char* linNear = "Pixelated\0"
                                    "Blurry\0"_j;

              magBase = imcxx::Combo("Interpolation when scaled up"_j, magBase,
                                     linNear);
              AUTO_PROP(samplers[i].mMagFilter, magBase);
              ImGui::Combo("Interpolation when scaled down"_j,
                           &min_filt.minBase, linNear);

              ImGui::Checkbox("Use mipmap"_j, &min_filt.mip);

              {
                ConditionalActive g(min_filt.mip);

                if (ImGui::CollapsingHeader("Mipmapping"_j,
                                            ImGuiTreeNodeFlags_DefaultOpen)) {
                  ImGui::Combo("Interpolation type"_j, &min_filt.minMipBase,
                               "Sudden\0"
                               "Smooth\0"_j);

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
          if (ImGui::BeginTabItem("Mapping"_j)) {
            const auto map_before = Mappings_from(
                matData.texMatrices[texmatrixid], matData.texGens[i]);
            auto map = map_before;

            map = DrawMappings(map);

            if (map != map_before) {
              auto mtx = matData.texMatrices[texmatrixid];
              auto gen = matData.texGens[i];
              Mappings_to(map, mtx, gen);
              AUTO_PROP(texMatrices[texmatrixid], mtx);
              AUTO_PROP(texGens[i], gen);
            }
            ImGui::EndTabItem();
          }
          if (ImGui::BeginTabItem("Mapping (Advanced Version)"_j)) {
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
                auto xfmodel = DrawCommXModel(tm->transformModel, is_j3d);
                AUTO_PROP(texMatrices[texmatrixid].transformModel, xfmodel);
              }
              {
                auto newMapMethod = DrawCommMapMethod(tm->method);
                AUTO_PROP(texMatrices[texmatrixid].method, newMapMethod);
              }
              if (is_j3d) {
                auto mod = DrawCommMapMod(tm->option);
                AUTO_PROP(texMatrices[texmatrixid].option, mod);
              }
            }
            ImGui::EndTabItem();
          }
          ImGui::EndTabBar();
        }
        ImGui::EndTabItem();
      }
    }

    for (std::size_t i = 0; i < matData.texGens.size(); ++i) {
      if (!open[i]) {
        auto ok = deleteSampler(matData, i);
        if (ok) {
          delegate.commit("Erased a sampler");
        } else {
          rsl::ErrorDialog("Could not delete sampler: "_j + ok.error());
        }
        break;
      }
    }
    ImGui::EndTabBar();
  }
}

kpi::RegisterPropertyView<IGCMaterial, SamplerSurface> SamplerSurfaceInstaller;

} // namespace libcube::UI
