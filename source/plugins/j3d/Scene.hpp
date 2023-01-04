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
  librii::gx::ColorSelChanApi rasOrder = librii::gx::ColorSelChanApi::null;
  u8 texMap = 0xFF, texCoord = 0xFF;

  bool operator==(const TevOrder& rhs) const noexcept = default;
};

struct SwapSel {
  u8 colorChanSel = 0, texSel = 0;

  bool operator==(const SwapSel& rhs) const noexcept = default;
};

using Tex = librii::j3d::Tex;

struct ModelData_ {
  struct Information {
    using ScalingRule = librii::j3d::ScalingRule;
    // For texmatrix calculations

    ScalingRule mScalingRule = ScalingRule::Basic;
    // nPacket, nVtx

    // Hierarchy data is included in joints.
  };
  Information info;
  bool isBDL = false;
  using Bufs = librii::j3d::Bufs;
  Bufs mBufs;

  struct Indirect {
    bool enabled = false;
    u8 nIndStage = 0;

    std::array<librii::gx::IndOrder, 4> tevOrder;
    std::array<librii::gx::IndirectMatrix, 3> texMtx;
    std::array<librii::gx::IndirectTextureScalePair, 4> texScale;
    std::array<librii::gx::TevStage::IndirectStage, 16> tevStage;

    Indirect() = default;
    Indirect(const librii::j3d::MaterialData& mat) {
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
  struct MatCache {
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
    void propagate(librii::j3d::MaterialData& mat);
  };

  MatCache mMatCache;
  std::vector<Tex> mTexCache;
};

struct ModelData : public virtual kpi::IObject, public ModelData_ {
  virtual ~ModelData() = default;
  // Shallow comparison
  bool operator==(const ModelData& rhs) const {
    // TODO: Check bufs
    return info.mScalingRule == rhs.info.mScalingRule;
  }

  std::string getName() const { return "Model"; }

  using Bufs = librii::j3d::Bufs;
  using MatCache = ModelData_::MatCache;
};

} // namespace riistudio::j3d

#include "Node.h"
