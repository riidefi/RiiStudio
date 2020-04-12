#pragma once

#include "PropertySupport.hpp"
#include <core/3d/i3dmodel.hpp>
#include <core/common.h>
#include <plugins/gc/GX/Material.hpp>

#include <core/kpi/Node.hpp>

#include "Texture.hpp"

namespace libcube {

// Assumption: all elements are contiguous--no holes
// Much faster than a vector for the many static sized arrays in materials
template <typename T, size_t N> struct array_vector : public std::array<T, N> {
  size_t size() const { return nElements; }
  size_t nElements = 0;

  void push_back(T elem) {
    std::array<T, N>::at(nElements) = std::move(elem);
    ++nElements;
  }
  void pop_back() { --nElements; }
  bool operator==(const array_vector& rhs) const noexcept {
    if (rhs.nElements != nElements)
      return false;
    for (int i = 0; i < nElements; ++i) {
      const T& l = this->at(i);
      const T& r = rhs.at(i);
      if (!(l == r))
        return false;
    }
    return true;
  }
};

template <typename T, std::size_t size>
struct copyable_polymorphic_array_vector
    : public array_vector<std::unique_ptr<T>, size> {
#ifdef _WIN32
  using super = array_vector;
#else
  // MSVC bug?
  using super = array_vector<std::unique_ptr<T>, size>;
#endif
  copyable_polymorphic_array_vector&
  operator=(const copyable_polymorphic_array_vector& rhs) {
    super::nElements = rhs.super::nElements;
    for (std::size_t i = 0; i < super::nElements; ++i) {
      super::at(i) = nullptr;
    }
    for (std::size_t i = 0; i < rhs.super::nElements; ++i) {
      super::at(i) = rhs.super::at(i)->clone();
    }
    return *this;
  }
  copyable_polymorphic_array_vector(
      const copyable_polymorphic_array_vector& rhs) {
    *this = rhs;
  }
  copyable_polymorphic_array_vector() = default;
};
struct GCMaterialData {
  // TODO:
  bool operator==(const GCMaterialData& rhs) const { return false; }

  std::string name;

  gx::CullMode cullMode;

  // Gen Info counts
  struct GenInfo {
    u8 nColorChan = 0;
    u8 nTexGen = 0;
    u8 nTevStage = 1;
    u8 nIndStage = 0;

    bool operator==(const GenInfo& rhs) const {
      return nColorChan == rhs.nColorChan && nTexGen == rhs.nTexGen &&
             nTevStage == rhs.nTevStage && nIndStage == rhs.nIndStage;
    }
  };
  GenInfo info;

  struct ChannelData {
    gx::Color matColor;
    gx::Color ambColor;

    bool operator==(const ChannelData& rhs) const {
      return matColor == rhs.matColor && ambColor == rhs.ambColor;
    }
  };
  array_vector<ChannelData, 2> chanData;
  // Color0, Alpha0, Color1, Alpha1
  array_vector<gx::ChannelControl, 4> colorChanControls;

  gx::Shader shader;

  array_vector<gx::TexCoordGen, 8> texGens;

  array_vector<gx::Color, 4> tevKonstColors;
  array_vector<gx::ColorS10, 4> tevColors;

  bool earlyZComparison = true;
  gx::ZMode zMode;

  // Split up -- only 3 indmtx
  std::vector<gx::IndirectTextureScalePair> mIndScales;
  std::vector<gx::IndirectMatrix> mIndMatrices;

  gx::AlphaComparison alphaCompare;
  gx::BlendMode blendMode;
  bool dither;
  enum class CommonMappingOption {
    NoSelection,
    DontRemapTextureSpace, // -1 -> 1 (J3D "basic")
    KeepTranslation        // Don't reset translation column
  };

  enum class CommonMappingMethod {
    // Shared
    Standard,
    EnvironmentMapping,

    // J3D name. This is G3D's only PROJMAP.
    ViewProjectionMapping,

    // J3D only by default. EGG adds this to G3D as "ManualProjectionMapping"
    ProjectionMapping,
    ManualProjectionMapping = ProjectionMapping,

    // G3D
    EnvironmentLightMapping,
    EnvironmentSpecularMapping,

    // J3D only?
    ManualEnvironmentMapping // Specify effect matrix maunally
                             // J3D 4/5?
  };
  enum class CommonTransformModel { Default, Maya, Max, XSI };
  struct TexMatrix {
    // TODO: Deprecate?
    gx::TexGenType projection =
        gx::TexGenType::Matrix2x4; // Only 3x4 and 2x4 valid

    glm::vec2 scale{1.0f, 1.0f};
    f32 rotate{0.0f};
    glm::vec2 translate{0.0f, 0.0f};

    std::array<f32, 16> effectMatrix{};

    CommonTransformModel transformModel = CommonTransformModel::Default;
    CommonMappingMethod method = CommonMappingMethod::Standard;
    CommonMappingOption option = CommonMappingOption::NoSelection;

    // G3d
    s8 camIdx = -1;
    s8 lightIdx = -1;

    virtual glm::mat3x4 compute(const glm::mat4& mdl, const glm::mat4& mvp);
    // TODO: Support / restriction

    virtual bool operator==(const TexMatrix& rhs) const {
      if (!(projection == rhs.projection && scale == rhs.scale &&
            rotate == rhs.rotate && translate == rhs.translate))
        return false;
      // if (effectMatrix != rhs.effectMatrix) return false;
      return transformModel == rhs.transformModel && method == rhs.method &&
             option == rhs.option && camIdx == rhs.camIdx &&
             lightIdx == rhs.lightIdx;
    }
    virtual std::unique_ptr<TexMatrix> clone() const {
      return std::make_unique<TexMatrix>(*this);
    }
    virtual ~TexMatrix() = default;
  };

  copyable_polymorphic_array_vector<TexMatrix, 10> texMatrices;

  struct SamplerData {
    std::string mTexture;
    std::string mPalette;

    gx::TextureWrapMode mWrapU = gx::TextureWrapMode::Repeat;
    gx::TextureWrapMode mWrapV = gx::TextureWrapMode::Repeat;

    // bool bMipMap = false;
    bool bEdgeLod;
    bool bBiasClamp;

    gx::AnisotropyLevel mMaxAniso;

    gx::TextureFilter mMinFilter = gx::TextureFilter::linear;
    gx::TextureFilter mMagFilter;
    f32 mLodBias = 0.0f;

    bool operator==(const SamplerData& rhs) const noexcept {
      return mTexture == rhs.mTexture && mWrapU == rhs.mWrapU &&
             mWrapV == rhs.mWrapV &&
             // bMipMap == rhs.bMipMap &&
             bEdgeLod == rhs.bEdgeLod && bBiasClamp == rhs.bBiasClamp &&
             mMaxAniso == rhs.mMaxAniso && mMinFilter == rhs.mMinFilter &&
             mMagFilter == rhs.mMagFilter && mLodBias == rhs.mLodBias;
    }
    virtual std::unique_ptr<SamplerData> clone() const {
      return std::make_unique<SamplerData>(*this);
    }
    virtual ~SamplerData() = default;
  };

  copyable_polymorphic_array_vector<SamplerData, 8> samplers;
};

struct IGCMaterial : public riistudio::lib3d::Material {
  // PX_TYPE_INFO("GC Material", "gc_mat", "GC::IMaterialDelegate");

  enum class Feature {
    CullMode,
    ZCompareLoc,
    ZCompare,
    GenInfo,

    MatAmbColor,

    Max
  };
  // Compat
  struct PropertySupport : public TPropertySupport<Feature> {
    using Feature = Feature;
    static constexpr std::array<const char*, (u64)Feature::Max> featureStrings =
        {"Culling Mode", "Early Z Comparison", "Z Comparison", "GenInfo",
         "Material/Ambient Colors"};
  };

  PropertySupport support;

  virtual GCMaterialData& getMaterialData() = 0;
  virtual const GCMaterialData& getMaterialData() const = 0;

  virtual const kpi::IDocumentNode* getParent() const { return nullptr; }
  std::pair<std::string, std::string> generateShaders() const override;
  void
  generateUniforms(DelegatedUBOBuilder& builder, const glm::mat4& M,
                   const glm::mat4& V, const glm::mat4& P, u32 shaderId,
                   const std::map<std::string, u32>& texIdMap) const override;

  virtual inline const kpi::FolderData* getTextureSource() const {
    // Assumption: Parent of parent model is a collection with children.
    const kpi::IDocumentNode* parent = getParent();
    assert(parent);
    const kpi::IDocumentNode* collection = parent->parent;
    assert(collection);

    return collection->getFolder<libcube::Texture>();
  }
  virtual const Texture& getTexture(const std::string& id) const = 0;
  void
  genSamplUniforms(u32 shaderId,
                   const std::map<std::string, u32>& texIdMap) const override;

  std::string getName() const override { return getMaterialData().name; }
  void setName(const std::string& name) override {
    getMaterialData().name = name;
  }

  void setMegaState(MegaState& state) const override;

  void configure(riistudio::lib3d::PixelOcclusion occlusion,
                 std::vector<std::string>& textures) override {
    // TODO
    if (textures.empty())
      return;

    const auto tex = textures[0];

    auto& mat = getMaterialData();

    auto sampler = std::make_unique<GCMaterialData::SamplerData>();
    sampler->mTexture = tex;
    // TODO: EdgeLod/BiasClamp
    // TODO: MaxAniso, filter, bias
    mat.samplers.push_back(std::move(sampler));
    mat.samplers.nElements = 1;

    mat.texGens.push_back(gx::TexCoordGen{
        gx::TexGenType::Matrix3x4, gx::TexGenSrc::UV0,
        gx::TexMatrix::TexMatrix0, false, gx::PostTexMatrix::Identity});
    mat.texGens.nElements = 1;
    auto mtx = std::make_unique<GCMaterialData::TexMatrix>();
    mtx->projection = gx::TexGenType::Matrix3x4;
    mtx->scale = {1, 1};
    mtx->rotate = 0;
    mtx->translate = {0, 0};
    mtx->effectMatrix = {0};
    mtx->transformModel = GCMaterialData::CommonTransformModel::Maya;
    mtx->method = GCMaterialData::CommonMappingMethod::Standard;
    mtx->option = GCMaterialData::CommonMappingOption::NoSelection;

    mat.texMatrices.push_back(std::move(mtx));
    mat.texMatrices.nElements = 1;

    mat.shader.mStages[0].texMap = 0;
    mat.shader.mStages[0].texCoord = 0;
    mat.shader.mStages[0].colorStage.d = gx::TevColorArg::texc;
    mat.shader.mStages[0].alphaStage.d = gx::TevAlphaArg::texa;
  }
};

} // namespace libcube
