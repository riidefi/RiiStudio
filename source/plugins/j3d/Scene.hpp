#pragma once

#include <core/3d/i3dmodel.hpp>

#include "Joint.hpp"
#include "Material.hpp"
#include "Shape.hpp"
#include "Texture.hpp"
#include "VertexBuffer.hpp"
#include <plugins/gc/Export/Scene.hpp>

namespace riistudio::j3d {

struct TevOrder {
  librii::gx::ColorSelChanApi rasOrder;
  u8 texMap, texCoord;

  bool operator==(const TevOrder& rhs) const noexcept = default;
};

struct SwapSel {
  u8 colorChanSel, texSel;

  bool operator==(const SwapSel& rhs) const noexcept = default;
};

struct Tex {
  librii::gx::TextureFormat mFormat;
  u8 transparency;
  u16 mWidth, mHeight;
  librii::gx::TextureWrapMode mWrapU, mWrapV;
  u8 mPaletteFormat;
  u16 nPalette;
  u32 ofsPalette = 0;
  u8 bMipMap;
  u8 bEdgeLod;
  u8 bBiasClamp;
  librii::gx::AnisotropyLevel mMaxAniso;
  librii::gx::TextureFilter mMinFilter;
  librii::gx::TextureFilter mMagFilter;
  s8 mMinLod;
  s8 mMaxLod;
  u8 mMipmapLevel;
  s16 mLodBias;
  u32 ofsTex = 0;

  // Not written, tracked
  s32 btiId = -1;

  bool operator==(const Tex& rhs) const {
    return mFormat == rhs.mFormat && transparency == rhs.transparency &&
           mWidth == rhs.mWidth && mHeight == rhs.mHeight &&
           mWrapU == rhs.mWrapU && mWrapV == rhs.mWrapV &&
           mPaletteFormat == rhs.mPaletteFormat && nPalette == rhs.nPalette &&
           ofsPalette == rhs.ofsPalette && bMipMap == rhs.bMipMap &&
           bEdgeLod == rhs.bEdgeLod && bBiasClamp == rhs.bBiasClamp &&
           mMaxAniso == rhs.mMaxAniso && mMinFilter == rhs.mMinFilter &&
           mMagFilter == rhs.mMagFilter && mMinLod == rhs.mMinLod &&
           mMaxLod == rhs.mMaxLod && mMipmapLevel == rhs.mMipmapLevel &&
           mLodBias == rhs.mLodBias && btiId == rhs.btiId; // ofsTex not checked
  }

  void transfer(oishii::BinaryReader& stream);
  void write(oishii::Writer& stream) const;
  Tex() = default;
  Tex(const Texture& data, const libcube::GCMaterialData::SamplerData& sampler);
};

struct ModelData : public virtual kpi::IObject {
  virtual ~ModelData() = default;
  // Shallow comparison
  bool operator==(const ModelData& rhs) const {
    // TODO: Check bufs
    return info.mScalingRule == rhs.info.mScalingRule;
  }
  struct Information {
    // For texmatrix calculations
    enum class ScalingRule { Basic, XSI, Maya };

    ScalingRule mScalingRule = ScalingRule::Basic;
    // nPacket, nVtx

    // Hierarchy data is included in joints.
  };

  std::string getName() const { return "Model"; }

  Information info;

  struct Bufs {
    // FIXME: Good default values
    VertexBuffer<glm::vec3, VBufferKind::position> pos{
        VQuantization{librii::gx::VertexComponentCount(
                          librii::gx::VertexComponentCount::Position::xyz),
                      librii::gx::VertexBufferType(
                          librii::gx::VertexBufferType::Generic::f32),
                      0, 0, 12}};
    VertexBuffer<glm::vec3, VBufferKind::normal> norm{
        VQuantization{librii::gx::VertexComponentCount(
                          librii::gx::VertexComponentCount::Normal::xyz),
                      librii::gx::VertexBufferType(
                          librii::gx::VertexBufferType::Generic::f32),
                      0, 0, 12}};
    std::array<VertexBuffer<librii::gx::Color, VBufferKind::color>, 2> color;
    std::array<VertexBuffer<glm::vec2, VBufferKind::textureCoordinate>, 8> uv;

    Bufs() {
      for (auto& clr : color) {
        clr = {VQuantization{
            librii::gx::VertexComponentCount(
                librii::gx::VertexComponentCount::Color::rgba),
            librii::gx::VertexBufferType(
                librii::gx::VertexBufferType::Color::FORMAT_32B_8888),
            0, 0, 4}};
      }
      for (auto& uv_ : uv) {
        uv_ = {VQuantization{
            librii::gx::VertexComponentCount(
                librii::gx::VertexComponentCount::TextureCoordinate::st),
            librii::gx::VertexBufferType(
                librii::gx::VertexBufferType::Generic::f32),
            0, 0, 8}};
      }
    }
  } mBufs = Bufs();

  bool isBDL = false;

  struct Indirect {
    bool enabled;
    u8 nIndStage;

    std::array<librii::gx::IndOrder, 4> tevOrder;
    std::array<librii::gx::IndirectMatrix, 3> texMtx;
    std::array<librii::gx::IndirectTextureScalePair, 4> texScale;
    std::array<librii::gx::TevStage::IndirectStage, 16> tevStage;

    Indirect() = default;
    Indirect(const MaterialData& mat) {
      enabled = mat.indEnabled;
      nIndStage = mat.indirectStages.size();

      assert(nIndStage <= 4);

      for (int i = 0; i < nIndStage; ++i)
        tevOrder[i] = mat.indirectStages[i].order;
      for (int i = 0; i < nIndStage; ++i)
        texScale[i] = mat.indirectStages[i].scale;

      for (int i = 0; i < 3 && i < mat.mIndMatrices.size(); ++i)
        texMtx[i] = mat.mIndMatrices[i];

      for (int i = 0; i < mat.mStages.size(); ++i)
        tevStage[i] = mat.mStages[i].indirectStage;
    }

    bool operator==(const Indirect& rhs) const noexcept = default;
  };
  mutable struct MatCache {
    template <typename T> using Section = std::vector<T>;
    Section<Indirect> indirectInfos;
    Section<librii::gx::CullMode> cullModes;
    Section<librii::gx::Color> matColors;
    Section<u8> nColorChan;
    Section<librii::gx::ChannelControl> colorChans;
    Section<librii::gx::Color> ambColors;
    Section<librii::gx::Color> lightColors;
    Section<u8> nTexGens;
    Section<librii::gx::TexCoordGen> texGens;
    Section<librii::gx::TexCoordGen> posTexGens;
    Section<Material::TexMatrix> texMatrices;
    Section<Material::TexMatrix> postTexMatrices;
    Section<Material::J3DSamplerData> samplers;
    Section<TevOrder> orders;
    Section<librii::gx::ColorS10> tevColors;
    Section<librii::gx::Color> konstColors;

    Section<u8> nTevStages;
    Section<librii::gx::TevStage> tevStages;
    Section<SwapSel> swapModes;
    Section<librii::gx::SwapTableEntry> swapTables;
    Section<librii::j3d::Fog> fogs;

    Section<librii::gx::AlphaComparison> alphaComparisons;
    Section<librii::gx::BlendMode> blendModes;
    Section<librii::gx::ZMode> zModes;
    Section<u8> zCompLocs;
    Section<u8> dithers;
    Section<librii::j3d::NBTScale> nbtScales;

    void clear() { *this = MatCache{}; }
    template <typename T>
    void update_section(std::vector<T>& sec, const T& data) {
      if (std::find(sec.begin(), sec.end(), data) == sec.end()) {
        sec.push_back(data);
      }
    }
    template <typename T, typename U>
    void update_section_multi(std::vector<T>& sec, const U& source) {
      for (int i = 0; i < source.size(); ++i) {
        update_section(sec, source[i]);
      }
    }
    void propagate(Material& mat);
  } mMatCache;

  mutable std::vector<Tex> mTexCache;
};

} // namespace riistudio::j3d

#include "Node.h"
