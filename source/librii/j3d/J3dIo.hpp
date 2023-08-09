#pragma once

#include <librii/j3d/data/JointData.hpp>
#include <librii/j3d/data/MaterialData.hpp>
#include <librii/j3d/data/ShapeData.hpp>
#include <librii/j3d/data/TextureData.hpp>
#include <librii/j3d/data/VertexData.hpp>

#include <plugins/gc/Export/Bone.hpp>

#include <LibBadUIFramework/Plugins.hpp> // LightIOTransaction

namespace librii::j3d {

struct TevOrder {
  librii::gx::ColorSelChanApi rasOrder = librii::gx::ColorSelChanApi::null;
  u8 texMap = 0xFF, texCoord = 0xFF;

  bool operator==(const TevOrder& rhs) const noexcept = default;
};

struct SwapSel {
  u8 colorChanSel = 0, texSel = 0;

  bool operator==(const SwapSel& rhs) const noexcept = default;
};

struct Indirect {
  bool enabled = false;
  u8 nIndStage = 0;

  std::array<librii::gx::IndOrder, 4> tevOrder;
  std::array<librii::gx::IndirectMatrix, 3> texMtx;
  std::array<librii::gx::IndirectTextureScalePair, 4> texScale;
  std::array<librii::gx::TevStage::IndirectStage, 16> tevStage;

  Indirect() = default;
  Indirect(const librii::j3d::MaterialData& mat) {
    // TODO: Check if any are actually used?
    enabled = mat.indirectStages.size() > 0;
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
  Section<MaterialData::TexMatrix> texMatrices;
  Section<MaterialData::TexMatrix> postTexMatrices;
  Section<MaterialData::J3DSamplerData> samplers;
  Section<librii::j3d::TevOrder> orders;
  Section<librii::gx::ColorS10> tevColors;
  Section<librii::gx::Color> konstColors;

  Section<u8> nTevStages;
  Section<librii::gx::TevStage> tevStages;
  Section<librii::j3d::SwapSel> swapModes;
  Section<librii::gx::SwapTableEntry> swapTables;
  Section<librii::j3d::Fog> fogs;

  Section<librii::gx::AlphaComparison> alphaComparisons;
  Section<librii::gx::BlendMode> blendModes;
  Section<librii::gx::ZMode> zModes;
  Section<u8> zCompLocs;
  Section<u8> dithers;
  Section<librii::j3d::NBTScale> nbtScales;

  bool operator==(const MatCache&) const = default;

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

struct J3dModel {
  ScalingRule scalingRule = ScalingRule::Basic;
  bool isBDL = false;
  Bufs vertexData;
  std::vector<libcube::DrawMatrix> drawMatrices;
  std::vector<librii::j3d::MaterialData> materials;
  std::vector<librii::j3d::JointData> joints;
  std::vector<librii::j3d::ShapeData> shapes;
  std::vector<librii::j3d::TextureData> textures;

  bool operator==(const J3dModel&) const = default;

  [[nodiscard]] static Result<J3dModel> fromFile(const std::string_view path,
                                                 kpi::LightIOTransaction& tx);
  [[nodiscard]] static Result<J3dModel> read(oishii::BinaryReader& reader,
                                             kpi::LightIOTransaction& tx);
  [[nodiscard]] Result<void> write(oishii::Writer& writer,
                                   bool print_linkmap = true);

  [[nodiscard]] Result<void> dropMtx();
  [[nodiscard]] Result<void> genMtx();
};

} // namespace librii::j3d
