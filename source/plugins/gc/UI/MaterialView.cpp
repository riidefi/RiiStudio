#include "TevSolver.hpp"                  // for optimizeNode
#include <algorithm>                      // for std::min
#include <core/3d/ui/Image.hpp>           // for ImagePreview
#include <core/kpi/PropertyView.hpp>      // for kpi::PropertyViewManager
#include <core/util/gui.hpp>              // for ImGui
#include <core/util/string_builder.hpp>   // for StringBuilder
#include <plugins/gc/Export/Material.hpp> // for GCMaterialData
#undef near
#undef max

// TODO: Awful interdependency. Fix this when icon system is stable
#include <frontend/editor/EditorWindow.hpp>

namespace libcube::UI {

using namespace riistudio::util;

auto get_material_data = [](auto& x) -> GCMaterialData& {
  return (GCMaterialData&)x.getMaterialData();
};

MAYBE_UNUSED auto mat_prop = [](auto& delegate, auto member,
                                const auto& after) {
  delegate.propertyEx(member, after, get_material_data);
};
auto mat_prop_ex = [](auto& delegate, auto member, auto draw) {
  delegate.propertyEx(member,
                      draw(delegate.getActive().getMaterialData().*member),
                      get_material_data);
};

struct CullMode {
  bool front, back;

  CullMode() : front(true), back(false) {}
  CullMode(bool f, bool b) : front(f), back(b) {}
  CullMode(librii::gx::CullMode c) { set(c); }

  void set(librii::gx::CullMode c) {
    switch (c) {
    case librii::gx::CullMode::All:
      front = back = false;
      break;
    case librii::gx::CullMode::None:
      front = back = true;
      break;
    case librii::gx::CullMode::Front:
      front = false;
      back = true;
      break;
    case librii::gx::CullMode::Back:
      front = true;
      back = false;
      break;
    default:
      assert(!"Invalid cull mode");
      front = back = true;
      break;
    }
  }
  librii::gx::CullMode get() const noexcept {
    if (front && back)
      return librii::gx::CullMode::None;

    if (!front && !back)
      return librii::gx::CullMode::All;

    return front ? librii::gx::CullMode::Back : librii::gx::CullMode::Front;
  }

  void draw() {
    ImGui::Text("Show sides of faces:");
    ImGui::Checkbox("Front", &front);
    ImGui::Checkbox("Back", &back);
  }
};
librii::gx::CullMode DrawCullMode(librii::gx::CullMode cull_mode) {
  CullMode widget(cull_mode);
  widget.draw();
  return widget.get();
}

struct DisplaySurface final {
  static inline const char* name = "Surface Visibility";
  static inline const char* icon = (const char*)ICON_FA_GHOST;
};
struct LightingSurface final {
  static inline const char* name = "Lighting";
  static inline const char* icon = (const char*)ICON_FA_SUN;
};
struct ColorSurface final {
  static inline const char* name = "Colors";
  static inline const char* icon = (const char*)ICON_FA_PAINT_BRUSH;
};
struct SamplerSurface final {
  static inline const char* name = "Samplers";
  static inline const char* icon = (const char*)ICON_FA_IMAGES;

  // Mark this surface to be more than an IDL tag.
  int tag_stateful;

  riistudio::frontend::ImagePreview mImg; // In mat sampler
  std::string mLastImg;
};
struct SwapTableSurface final {
  static inline const char* name = "Swap Tables";
  static inline const char* icon = (const char*)ICON_FA_SWATCHBOOK;
};
struct StageSurface final {
  static inline const char* name = "Stage";
  static inline const char* icon = (const char*)ICON_FA_NETWORK_WIRED;

  // Mark this surface to be more than an IDL tag.
  int tag_stateful;

  riistudio::frontend::ImagePreview mImg; // In mat sampler
  std::string mLastImg;
};
struct FogSurface final {
  static inline const char* name = "Fog";
  static inline const char* icon = (const char*)ICON_FA_GHOST;
};
struct PixelSurface final {
  static inline const char* name = "Pixel";
  static inline const char* icon = (const char*)ICON_FA_GHOST;

  int tag_stateful;

  std::string force_custom_at; // If empty, disabled: Name of material
  std::string force_custom_whole;
};

void drawProperty(kpi::PropertyDelegate<IGCMaterial>& delegate,
                  DisplaySurface) {
  mat_prop_ex(delegate, &GCMaterialData::cullMode, DrawCullMode);
}
#define _AUTO_PROPERTY(delegate, before, after, val)                           \
  delegate.property(                                                           \
      before, after, [&](const auto& x) { return x.val; },                     \
      [&](auto& x, auto& y) {                                                  \
        x.val = y;                                                             \
        x.notifyObservers();                                                   \
      })

#define AUTO_PROP(before, after)                                               \
  _AUTO_PROPERTY(delegate, delegate.getActive().getMaterialData().before,      \
                 after, getMaterialData().before)
void drawProperty(kpi::PropertyDelegate<IGCMaterial>& delegate, ColorSurface) {
  librii::gx::ColorF32 clr;
  auto& matData = delegate.getActive().getMaterialData();

  if (ImGui::CollapsingHeader("TEV Color Registers",
                              ImGuiTreeNodeFlags_DefaultOpen)) {

    // TODO: Is CPREV default state accessible?
    for (std::size_t i = 0; i < 4; ++i) {
      clr = matData.tevColors[i];
      ImGui::ColorEdit4(
          (std::string("Color Register ") + std::to_string(i)).c_str(), clr);
      clr.clamp(-1024.0f / 255.0f, 1023.0f / 255.0f);
      AUTO_PROP(tevColors[i], static_cast<librii::gx::ColorS10>(clr));
    }
  }

  if (ImGui::CollapsingHeader("TEV Constant Colors",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    for (std::size_t i = 0; i < 4; ++i) {
      clr = matData.tevKonstColors[i];
      ImGui::ColorEdit4(
          (std::string("Constant Register ") + std::to_string(i)).c_str(), clr);
      clr.clamp(0.0f, 1.0f);
      AUTO_PROP(tevKonstColors[i], static_cast<librii::gx::Color>(clr));
    }
  }
}

bool IconSelectable(const char* text, bool selected,
                    const riistudio::lib3d::Texture* tex,
                    riistudio::frontend::EditorWindow* ed) {
  ImGui::PushID(tex->getName().c_str());
  auto s =
      ImGui::Selectable("##ID", selected, ImGuiSelectableFlags_None, {0, 32});
  ImGui::PopID();
  if (ed != nullptr) {
    ImGui::SameLine();
    ed->drawImageIcon(tex, 32);
  }
  ImGui::SameLine();
  ImGui::TextUnformatted(text);
  return s;
}

std::string TextureImageCombo(const char* current,
                              kpi::ConstCollectionRange<Texture> images,
                              riistudio::frontend::EditorWindow* ed) {
  std::string result;
  if (ImGui::BeginCombo("Name##Img", current)) {
    for (const auto& tex : images) {
      bool selected = tex.getName() == current;
      if (IconSelectable(tex.getName().c_str(), selected, &tex, ed)) {
        result = tex.getName();
      }
      if (selected)
        ImGui::SetItemDefaultFocus();
    }

    ImGui::EndCombo();
  }
  return result;
}

int SamplerCombo(
    int current,
    copyable_polymorphic_array_vector<GCMaterialData::SamplerData, 8>& samplers,
    kpi::ConstCollectionRange<riistudio::lib3d::Texture> images,
    riistudio::frontend::EditorWindow* ed, bool allow_none = false) {
  int result = current;

  const auto format = [&](int id) -> std::string {
    if (id >= samplers.size())
      return "No selection";
    return std::string("[") + std::to_string(id) + "] " +
           samplers[id]->mTexture;
  };

  if (ImGui::BeginCombo("Sampler ID##Img", samplers.empty()
                                               ? "No Samplers"
                                               : format(current).c_str())) {
    if (allow_none) {
      bool selected = current = 0xff;
      if (ImGui::Selectable("None", selected)) {
        result = 0xff;
      }
      if (selected)
        ImGui::SetItemDefaultFocus();
    }

    for (int i = 0; i < samplers.size(); ++i) {
      bool selected = i == current;

      const riistudio::lib3d::Texture* curImg = nullptr;

      for (auto& it : images) {
        if (it.getName() == samplers[i]->mTexture) {
          curImg = &it;
        }
      }

      if (IconSelectable(format(i).c_str(), selected, curImg, ed)) {
        result = i;
      }
      if (selected)
        ImGui::SetItemDefaultFocus();
    }

    ImGui::EndCombo();
  }
  return result;
}

void drawProperty(kpi::PropertyDelegate<IGCMaterial>& delegate,
                  SamplerSurface& surface) {
  auto& matData = delegate.getActive().getMaterialData();

  if (matData.info.nTexGen != matData.texGens.size() ||
      matData.info.nTexGen != matData.samplers.size()) {
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
      d.texMatrices.push_back(std::make_unique<GCMaterialData::TexMatrix>());
      d.samplers.push_back(std::make_unique<GCMaterialData::SamplerData>());
      ++d.info.nTexGen;
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
        tm = matData.texMatrices[texmatrixid].get(); // TODO: Proper lookup
      auto& samp = matData.samplers[i];

      const auto mImgs = delegate.getActive().getTextureSource();
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
                    texMatrices[texmatrixid]->transformModel,
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
                AUTO_PROP(texMatrices[texmatrixid]->method, newMapMethod);

                int mod = static_cast<int>(tm->option);
                ImGui::Combo(
                    "Option", &mod,
                    "Standard\0J3D Basic: Don't remap into texture space (Keep "
                    "-1..1 not 0...1)\0J3D Old: Keep translation column.");
                AUTO_PROP(
                    texMatrices[texmatrixid]->option,
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
            AUTO_PROP(texMatrices[texmatrixid]->scale, s);
            if (r != rotate)
              AUTO_PROP(texMatrices[texmatrixid]->rotate, glm::radians(r));
            AUTO_PROP(texMatrices[texmatrixid]->translate, t);
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
      --matData.info.nTexGen;

      // Correct stages
      for (auto& stage : matData.shader.mStages) {
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
void drawProperty(kpi::PropertyDelegate<IGCMaterial>& delegate,
                  SwapTableSurface) {
  auto& matData = delegate.getActive().getMaterialData();

  const char* colors = "Sample from Red\0Sample from Green\0Sample from "
                       "Blue\0Sample from Alpha\0";
  ImGui::BeginTable("Swap Tables", 5, ImGuiTableFlags_Borders);
  ImGui::TableSetupColumn("Table ID");
  ImGui::TableSetupColumn("Red Destination");
  ImGui::TableSetupColumn("Green Destination");
  ImGui::TableSetupColumn("Blue Destination");
  ImGui::TableSetupColumn("Alpha Destination");
  ImGui::TableAutoHeaders();
  ImGui::TableNextRow();
  for (int i = 0; i < matData.shader.mSwapTable.size(); ++i) {
    ImGui::PushID(i);
    auto& swap = matData.shader.mSwapTable[i];

    int r = static_cast<int>(swap.r);
    int g = static_cast<int>(swap.g);
    int b = static_cast<int>(swap.b);
    int a = static_cast<int>(swap.a);

    ImGui::Text("Swap %i", i);
    ImGui::TableNextCell();

    ImGui::Combo("##R", &r, colors);
    ImGui::TableNextCell();
    ImGui::Combo("##G", &g, colors);
    ImGui::TableNextCell();
    ImGui::Combo("##B", &b, colors);
    ImGui::TableNextCell();
    ImGui::Combo("##A", &a, colors);
    ImGui::TableNextCell();

    AUTO_PROP(shader.mSwapTable[i].r, static_cast<gx::ColorComponent>(r));
    AUTO_PROP(shader.mSwapTable[i].g, static_cast<gx::ColorComponent>(g));
    AUTO_PROP(shader.mSwapTable[i].b, static_cast<gx::ColorComponent>(b));
    AUTO_PROP(shader.mSwapTable[i].a, static_cast<gx::ColorComponent>(a));

    ImGui::PopID();
    ImGui::TableNextRow();
  }

  ImGui::EndTable();
}

const char* colorOpt =
    "Register 3 Color\0Register 3 Alpha\0Register 0 "
    "Color\0Register 0 Alpha\0Register 1 Color\0Register 1 "
    "Alpha\0Register 2 Color\0Register 2 Alpha\0Texture "
    "Color\0Texture Alpha\0Raster Color\0Raster Alpha\0 1.0\0 "
    "0.5\0 Constant Color Selection\0 0.0\0\0";
const char* alphaOpt = "Register 3 Alpha\0Register 0 Alpha\0Register 1 "
                       "Alpha\0Register 2 Alpha\0Texture Alpha\0Raster "
                       "Alpha\0Constant Alpha Selection\0 0.0\0\0";

#if 0
	ImGui::PushItemWidth(200);
	ImGui::Text("[");
	ImGui::SameLine();
	ImGui::Combo("##D", &d, colorOpt);
	ImGui::SameLine();
	ImGui::Text("{(1 - ");
	{
		ConditionalActive g(false);
		ImGui::SameLine();
		ImGui::Combo("##C_", &c, colorOpt);
	}
	ImGui::SameLine();
	ImGui::Text(") * ");
	ImGui::SameLine();
	ImGui::Combo("##A", &a, colorOpt);

	ImGui::SameLine();
	ImGui::Text(" + ");
	ImGui::SameLine();
	ImGui::Combo("##C", &c, colorOpt);
	ImGui::SameLine();
	ImGui::Text(" * ");
	ImGui::SameLine();
	ImGui::Combo("##B", &b, colorOpt);
	ImGui::SameLine();
	ImGui::Text(" } ");
#endif

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

  if (matData.info.nTevStage != matData.shader.mStages.size()) {
    ImGui::Text("Cannot edit this material: source data is invalid");
    return;
  }

  auto drawStage = [&](librii::gx::TevStage& stage, int i) {
#define STAGE_PROP(a, b) AUTO_PROP(shader.mStages[i].a, b)
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
                             delegate.getActive().getTextureSource(),
                             delegate.mEd, true);
        if (texid != stage.texMap) {
          for (auto* e : delegate.mAffected) {
            auto& stage = e->getMaterialData().shader.mStages[i];
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

        const auto mImgs = delegate.getActive().getTextureSource();
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

  auto& stages = matData.shader.mStages;

  if (ImGui::Button("Add a Stage")) {
    stages.emplace_back();
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
    stages.erase(stages.begin() + i);

    if (stages.empty()) {
      stages.emplace_back();
    }

    break;
  }

  matData.info.nTevStage = matData.shader.mStages.size();

  if (changed)
    delegate.commit("Stage added/removed");
}

void drawProperty(kpi::PropertyDelegate<IGCMaterial>& delegate, FogSurface) {
  // auto& matData = delegate.getActive().getMaterialData();
}
void drawProperty(kpi::PropertyDelegate<IGCMaterial>& delegate,
                  LightingSurface) {
  auto& matData = delegate.getActive().getMaterialData();

  // Color0Alpha0, Color1Alpha1
  auto& colors = matData.chanData;
  // Color0, Alpha0, Color1, Alpha1
  auto& controls = matData.colorChanControls;

  // ImGui::Text("Number of colors:   %u", colors.size());
  // ImGui::Text("Number of controls: %u", controls.size());

  if (colors.size() > controls.size() / 2 || controls.size() % 2 != 0 ||
      controls.size() == 0) {
    ImGui::Text("Cannot edit this material's lighting data.");
    return;
  }

  const auto draw_color_channel = [&](int i) {
    auto& ctrl = controls[i];

    ImGui::CollapsingHeader(i % 2 ? "Alpha Channel" : "Color Channel",
                            ImGuiTreeNodeFlags_DefaultOpen);

    {
      auto diffuse_src = ctrl.Material;
      bool vclr = diffuse_src == gx::ColorSource::Vertex;
      ImGui::Checkbox(i % 2 ? "Vertex Alpha" : "Vertex Colors", &vclr);
      diffuse_src = vclr ? gx::ColorSource::Vertex : gx::ColorSource::Register;
      AUTO_PROP(colorChanControls[i].Material, diffuse_src);

      {
        ConditionalActive g(!vclr);

        librii::gx::ColorF32 mclr = colors[i / 2].matColor;
        if (i % 2 == 0) {
          ImGui::ColorEdit3("Diffuse Color", mclr);
        } else {
          ImGui::DragFloat("Diffuse Alpha", &mclr.a, 0.0f, 1.0f);
        }
        AUTO_PROP(chanData[i / 2].matColor, (librii::gx::Color)mclr);
      }
    }
    bool enabled = ctrl.enabled;
    ImGui::Checkbox("Affected by Light", &enabled);
    AUTO_PROP(colorChanControls[i].enabled, enabled);

    ConditionalActive g(enabled);
    {
      auto amb_src = ctrl.Ambient;
      bool vclr = amb_src == gx::ColorSource::Vertex;
      ImGui::Checkbox(i % 2 ? "Ambient Alpha uses Vertex Alpha"
                            : "Ambient Color uses Vertex Colors",
                      &vclr);
      amb_src = vclr ? gx::ColorSource::Vertex : gx::ColorSource::Register;
      AUTO_PROP(colorChanControls[i].Ambient, amb_src);

      {
        ConditionalActive g(!vclr);

        librii::gx::ColorF32 aclr = colors[i / 2].ambColor;
        if (i % 2 == 0) {
          ImGui::ColorEdit3("Ambient Color", aclr);
        } else {
          ImGui::DragFloat("Ambient Alpha", &aclr.a, 0.0f, 1.0f);
        }
        AUTO_PROP(chanData[i / 2].ambColor, (librii::gx::Color)aclr);
      }
    }

    int diffuse_fn = static_cast<int>(ctrl.diffuseFn);
    int atten_fn = static_cast<int>(ctrl.attenuationFn);

    ImGui::Combo("Diffusion Type", &diffuse_fn, "None\0Signed\0Clamped\0");
    AUTO_PROP(colorChanControls[i].diffuseFn,
              static_cast<librii::gx::DiffuseFunction>(diffuse_fn));
    ImGui::Combo("Attenuation Type", &atten_fn,
                 "Specular\0Spotlight (Diffuse)\0None\0None*\0");
    AUTO_PROP(colorChanControls[i].attenuationFn,
              static_cast<librii::gx::AttenuationFunction>(atten_fn));

    ImGui::Text("Enabled Lights:");
    auto light_mask = static_cast<u32>(ctrl.lightMask);
    ImGui::BeginTable("Light Mask", 8, ImGuiTableFlags_Borders);
    for (int i = 0; i < 8; ++i) {
      char header[2]{0};
      header[0] = '0' + i;
      ImGui::TableSetupColumn(header);
    }
    ImGui::TableAutoHeaders();
    ImGui::TableNextRow();
    for (int i = 0; i < 8; ++i) {
      bool light_enabled = (static_cast<u32>(ctrl.lightMask) & (1 << i)) != 0;
      ImGui::TableSetColumnIndex(i);
      ImGui::PushID(i);
      ImGui::Checkbox("###TableLightMask", &light_enabled);
      ImGui::PopID();

      if (light_enabled) {
        light_mask |= (1 << i);
      } else {
        light_mask &= ~(1 << i);
      }
    }
    ImGui::EndTable();
    AUTO_PROP(colorChanControls[i].lightMask,
              static_cast<librii::gx::LightID>(light_mask));
  };

  for (int i = 0; i < controls.size(); i += 2) {
    IDScope g(i);
    if (ImGui::CollapsingHeader(
            (std::string("Channel ") + std::to_string(i / 2)).c_str(),
            ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Columns(2);
      {
        IDScope g(i);
        draw_color_channel(i);
      }
      ImGui::NextColumn();
      {
        IDScope g(i + 1);
        draw_color_channel(i + 1);
      }
      ImGui::EndColumns();
    }
  }
  matData.info.nColorChan = controls.size() >= 2 //
                                ? 2
                                : controls.size() > 0 ? 1 : 0;
}
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

void installPolygonView();

kpi::DecentralizedInstaller Installer([](kpi::ApplicationPlugins& installer) {
  kpi::PropertyViewManager& manager = kpi::PropertyViewManager::getInstance();
  manager.addPropertyView<libcube::IGCMaterial, DisplaySurface>();
  manager.addPropertyView<libcube::IGCMaterial, LightingSurface>();
  manager.addPropertyView<libcube::IGCMaterial, ColorSurface>();
  manager.addPropertyView<libcube::IGCMaterial, SamplerSurface>();
  manager.addPropertyView<libcube::IGCMaterial, SwapTableSurface>();
  manager.addPropertyView<libcube::IGCMaterial, StageSurface>();
  // manager.addPropertyView<libcube::IGCMaterial, FogSurface>();
  manager.addPropertyView<libcube::IGCMaterial, PixelSurface>();

  installPolygonView();
});

} // namespace libcube::UI
