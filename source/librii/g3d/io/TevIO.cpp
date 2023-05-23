#include "TevIO.hpp"
#include <librii/gpu/DLBuilder.hpp>
#include <librii/gpu/DLInterpreter.hpp>
#include <librii/gpu/DLPixShader.hpp>

#include "MatIO.hpp"

// Note: C++23 specifies just `import std;`
IMPORT_STD;

namespace librii::g3d {

const std::array<u32, 16> shaderDlSizes{
    160, //  0
    160, //  1
    192, //  2
    192, //  3
    256, //  4
    256, //  5
    288, //  6
    288, //  7
    352, //  8
    352, //  9
    384, // 10
    384, // 11
    448, // 12
    448, // 13
    480, // 14
    480, // 15
};

BinaryTev toBinaryTev(const G3dShader& sh, u32 tev_id) {
  std::array<u8, 8> lut;
  // texgens and maps are coupled
  std::fill(lut.begin(), lut.end(), 0xff);
  for (auto& stage : sh.mStages)
    if (stage.texCoord != 0xff && stage.texMap != 0xff)
      lut[stage.texCoord] = stage.texMap;
  for (auto& order : sh.mIndirectOrders)
    if (order.refCoord != 0xff && order.refMap != 0xff)
      lut[order.refCoord] = order.refMap;
  return BinaryTev{.id = tev_id,
                   .reserved = {},
                   .coordToMapLut = lut,
                   .reserved2 = {},
                   .dl = BinaryTevDL{
                       .swapTable = sh.mSwapTable,
                       .indOrders = sh.mIndirectOrders,
                       .tevStages = sh.mStages,
                   }};
}

void BinaryTevDL::write(oishii::Writer& writer) const {
  auto dl_start = writer.tell();
  librii::gpu::DLBuilder dl(writer);
  for (int i = 0; i < 4; ++i)
    dl.setTevSwapModeTable(i, swapTable[i]);
  rsl::array_vector<gx::IndOrder, 4> ind_orders;
  ind_orders.resize(4);
  std::fill(ind_orders.begin(), ind_orders.end(), gx::NullOrder);
  std::copy(indOrders.begin(), indOrders.end(), ind_orders.begin());
  dl.setIndTexOrder(ind_orders[0].refCoord, ind_orders[0].refMap,
                    ind_orders[1].refCoord, ind_orders[1].refMap,
                    ind_orders[2].refCoord, ind_orders[2].refMap,
                    ind_orders[3].refCoord, ind_orders[3].refMap);
  dl.align(); // 11

  assert(writer.tell() - dl_start == 0x60);

  std::array<librii::gx::TevStage, 16> stages;
  for (int i = 0; i < tevStages.size(); ++i)
    stages[i] = tevStages[i];

  const auto stages_count = tevStages.size();
  const auto stages_count_rounded = roundUp(stages_count, 2);

  for (unsigned i = 0; i < stages_count_rounded; i += 2) {
    const librii::gx::TevStage even_stage = stages[i];
    librii::gx::TevStage odd_stage;
    odd_stage.texCoord = 7;
    odd_stage.texMap = 0xFF; // map = 0, enable = 0
    odd_stage.rasOrder = librii::gx::ColorSelChanApi::null;

    if (i + 1 < tevStages.size()) {
      odd_stage = stages[i + 1];
    }

    const auto couple_start = writer.tell();

    dl.setTevKonstantSel(
        i, even_stage.colorStage.constantSelection,
        even_stage.alphaStage.constantSelection,
        i + 1 < stages_count ? odd_stage.colorStage.constantSelection
                             : librii::gx::TevKColorSel::const_8_8,
        i + 1 < stages_count ? odd_stage.alphaStage.constantSelection
                             : librii::gx::TevKAlphaSel::const_8_8);

    dl.setTevOrder(i, even_stage, odd_stage);

    dl.setTevColorCalc(i, even_stage.colorStage);
    if (i + 1 < stages_count)
      dl.setTevColorCalc(i + 1, odd_stage.colorStage);
    else
      writer.skip(5);

    dl.setTevAlphaCalcAndSwap(i, even_stage.alphaStage, even_stage.rasSwap,
                              even_stage.texMapSwap);
    if (i + 1 < stages_count)
      dl.setTevAlphaCalcAndSwap(i + 1, odd_stage.alphaStage, odd_stage.rasSwap,
                                odd_stage.texMapSwap);
    else
      writer.skip(5);

    dl.setTevIndirect(i, even_stage.indirectStage);
    if (i + 1 < stages_count)
      dl.setTevIndirect(i + 1, odd_stage.indirectStage);
    else
      writer.skip(5);

    writer.skip(3); // 3

    MAYBE_UNUSED const auto couple_len = writer.tell() - couple_start;
    rsl::trace("CoupleLen: {}", couple_len);
    assert(couple_len == 48);
  }
  auto blank_dl = 48 * (8 - stages_count_rounded / 2);
  writer.reserveNext(blank_dl);
  writer.skip(blank_dl);

  MAYBE_UNUSED const auto end = writer.tell();
  MAYBE_UNUSED const auto tev_size = writer.tell() - dl_start + 32;
  assert(tev_size == 0x200);
}

Result<void> BinaryTev::read(oishii::BinaryReader& reader,
                             unsigned int tev_addr, bool brawlbox_bug) {
  librii::gx::LowLevelGxMaterial mat;
  // So the DL parser doesn't discard IndOrders
  mat.indirectStages.resize(4);
  TRY(ReadTev(mat, reader, tev_addr, /* trust_stagecount */ true,
              brawlbox_bug));
  id = TRY(reader.tryGetAt<u32>(tev_addr + 8));
  // TODO: LUT is skipped among other fields
  G3dShader sh(mat);
  reserved = {};
  coordToMapLut = {};
  reserved2 = {};
  dl.swapTable = sh.mSwapTable;
  dl.indOrders = sh.mIndirectOrders;
  dl.tevStages = sh.mStages;
  return {};
}
void BinaryTev::writeBody(oishii::Writer& writer) const {
  // For debug tracking
  MAYBE_UNUSED auto tev_start = writer.tell() - 8;

  writer.write<u32>(id);
  writer.write<u8>(dl.tevStages.size());
  for (auto e : reserved)
    writer.write<u8>(e);

  for (auto e : coordToMapLut)
    writer.write<u8>(e);

  for (auto e : reserved2)
    writer.write<u8>(e);

  // DL
  assert(writer.tell() - tev_start == 32);
  dl.write(writer);
}

Result<void> ReadTev(librii::gx::LowLevelGxMaterial& mat,
                     oishii::BinaryReader& reader, unsigned int tev_addr,
                     bool trust_stagecount, bool brawlbox_bug) {
  if (trust_stagecount) {
    reader.seekSet(tev_addr + 4);
    const u8 num_stages = TRY(reader.tryRead<u8>());
    EXPECT(num_stages <= 16);
    mat.mStages.resize(num_stages);
  }
  reader.seekSet(tev_addr + 16);
  std::array<u8, 8> coord_map_lut;
  for (auto& e : coord_map_lut)
    e = TRY(reader.tryRead<u8>());
  bool error = false;
  {
    u8 last = 0;
    for (auto e : coord_map_lut) {
      error |= e < last;
      last = e;
    }
  }
  if (error) {
    rsl::error("Invalid sampler configuration?");
  }

  if (!brawlbox_bug) {
    reader.seekSet(tev_addr + 32);
  } else {
    // BrawlBox Behavior (and part of every .mdl0shade file)
    // It aligns to 32B in absolute terms, so we are actually +24 when we
    // consider the custom header.
    reader.seekSet(tev_addr + 24);
  }
  {
    librii::gpu::QDisplayListShaderHandler shaderHandler(mat,
                                                         mat.mStages.size());
    TRY(librii::gpu::RunDisplayList(reader, shaderHandler,
                                    shaderDlSizes[mat.mStages.size()]));
  }

  return {};
}

void WriteTevBody(oishii::Writer& writer, u32 tev_id, const G3dShader& tev) {
  BinaryTev bin = toBinaryTev(tev, tev_id);
  bin.writeBody(writer);
}

} // namespace librii::g3d
