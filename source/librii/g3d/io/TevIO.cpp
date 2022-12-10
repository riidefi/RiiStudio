#include "TevIO.hpp"
#include <algorithm>
#include <array>
#include <librii/gpu/DLBuilder.hpp>
#include <librii/gpu/DLInterpreter.hpp>
#include <librii/gpu/DLPixShader.hpp>

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

void ReadTev(librii::gx::LowLevelGxMaterial& mat, oishii::BinaryReader& reader,
             unsigned int tev_addr, bool trust_stagecount, bool brawlbox_bug) {
  if (trust_stagecount) {
    reader.seekSet(tev_addr + 4);
    const u8 num_stages = reader.read<u8>();
    assert(num_stages <= 16);
    mat.mStages.resize(num_stages);
  }
  reader.seekSet(tev_addr + 16);
  std::array<u8, 8> coord_map_lut;
  for (auto& e : coord_map_lut)
    e = reader.read<u8>();
  // printf(">>>>> Coord->Map LUT:\n");
  // for (auto e : coord_map_lut)
  //   printf("%u ", (unsigned)e);
  // printf("\n");
  bool error = false;
  {
    u8 last = 0;
    for (auto e : coord_map_lut) {
      error |= e <= last;
      last = e;
    }
  }
  // assert(!error && "Invalid sampler configuration");
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
    librii::gpu::RunDisplayList(reader, shaderHandler,
                                shaderDlSizes[mat.mStages.size()]);
  }
}

void WriteTevBody(oishii::Writer& writer, u32 tev_id, const G3dShader& tev) {
  // For debug tracking
  MAYBE_UNUSED auto tev_start = writer.tell() - 8;

  writer.write<u32>(tev_id);
  writer.write<u8>(tev.mStages.size());
  writer.skip(3);

  // texgens and maps are coupled
  std::array<u8, 8> genTexMapping;

  std::fill(genTexMapping.begin(), genTexMapping.end(), 0xff);
  for (auto& stage : tev.mStages)
    if (stage.texCoord != 0xff && stage.texMap != 0xff)
      genTexMapping[stage.texCoord] = stage.texMap;
  for (auto& order : tev.mIndirectOrders)
    if (order.refCoord != 0xff && order.refMap != 0xff)
      genTexMapping[order.refCoord] = order.refMap;

  for (auto e : genTexMapping)
    writer.write<u8>(e);

  writer.skip(8);

  // DL
  assert(writer.tell() - tev_start == 32);
  librii::gpu::DLBuilder dl(writer);
  for (int i = 0; i < 4; ++i)
    dl.setTevSwapModeTable(i, tev.mSwapTable[i]);
  rsl::array_vector<gx::IndOrder, 4> ind_orders;
  ind_orders.resize(4);
  std::fill(ind_orders.begin(), ind_orders.end(), gx::NullOrder);
  std::copy(tev.mIndirectOrders.begin(), tev.mIndirectOrders.end(),
            ind_orders.begin());
  dl.setIndTexOrder(ind_orders[0].refCoord, ind_orders[0].refMap,
                    ind_orders[1].refCoord, ind_orders[1].refMap,
                    ind_orders[2].refCoord, ind_orders[2].refMap,
                    ind_orders[3].refCoord, ind_orders[3].refMap);
  dl.align(); // 11

  assert(writer.tell() - tev_start == 32 + 0x60);

  std::array<librii::gx::TevStage, 16> stages;
  for (int i = 0; i < tev.mStages.size(); ++i)
    stages[i] = tev.mStages[i];

  const auto stages_count = tev.mStages.size();
  const auto stages_count_rounded = roundUp(stages_count, 2);

  for (unsigned i = 0; i < stages_count_rounded; i += 2) {
    const librii::gx::TevStage even_stage = stages[i];
    librii::gx::TevStage odd_stage;
    odd_stage.texCoord = 7;
    odd_stage.texMap = 0xFF; // map = 0, enable = 0
    odd_stage.rasOrder = librii::gx::ColorSelChanApi::null;

    if (i + 1 < tev.mStages.size()) {
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
    DebugReport("CoupleLen: %u\n", (unsigned)couple_len);
    assert(couple_len == 48);
  }
  auto blank_dl = 48 * (8 - stages_count_rounded / 2);
  writer.reserveNext(blank_dl);
  writer.skip(blank_dl);

  MAYBE_UNUSED const auto end = writer.tell();
  MAYBE_UNUSED const auto tev_size = writer.tell() - tev_start;
  assert(tev_size == 0x200);
}

} // namespace librii::g3d
