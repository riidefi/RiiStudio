#include "DLPixShader.hpp"

namespace librii::gpu {

QDisplayListShaderHandler::QDisplayListShaderHandler(
    gx::LowLevelGxMaterial& material, int numStages)
    : mMaterial(material), mNumStages(numStages) {}
QDisplayListShaderHandler::~QDisplayListShaderHandler() = default;

Result<void> QDisplayListShaderHandler::onCommandBP(const QBPCommand& token) {
  switch ((u32)token.reg) {
  case (u32)BPAddress::TEV_KSEL + (0 * 2):
  case (u32)BPAddress::TEV_KSEL + (0 * 2) + 1:
  case (u32)BPAddress::TEV_KSEL + (1 * 2):
  case (u32)BPAddress::TEV_KSEL + (1 * 2) + 1:
  case (u32)BPAddress::TEV_KSEL + (2 * 2):
  case (u32)BPAddress::TEV_KSEL + (2 * 2) + 1:
  case (u32)BPAddress::TEV_KSEL + (3 * 2):
  case (u32)BPAddress::TEV_KSEL + (3 * 2) + 1:
    mGpuShader.setReg<TevKSel>(
        mGpuShader.tevksel[(u32)token.reg - (u32)BPAddress::TEV_KSEL], token);
    break;
  case BPAddress::IREF:
    mGpuShader.setReg<RAS1_IREF>(mGpuShader.iref, token);
    break;
  case (u32)BPAddress::TREF + 0:
  case (u32)BPAddress::TREF + 1:
  case (u32)BPAddress::TREF + 2:
  case (u32)BPAddress::TREF + 3:
  case (u32)BPAddress::TREF + 4:
  case (u32)BPAddress::TREF + 5:
  case (u32)BPAddress::TREF + 6:
  case (u32)BPAddress::TREF + 7:
    mGpuShader.setReg<RAS1_TREF>(
        mGpuShader.tref[(u32)token.reg - (u32)BPAddress::TREF], token);
    break;
  case (u32)BPAddress::TEV_COLOR_ENV + 0 * 2:
  case (u32)BPAddress::TEV_COLOR_ENV + 1 * 2:
  case (u32)BPAddress::TEV_COLOR_ENV + 2 * 2:
  case (u32)BPAddress::TEV_COLOR_ENV + 3 * 2:
  case (u32)BPAddress::TEV_COLOR_ENV + 4 * 2:
  case (u32)BPAddress::TEV_COLOR_ENV + 5 * 2:
  case (u32)BPAddress::TEV_COLOR_ENV + 6 * 2:
  case (u32)BPAddress::TEV_COLOR_ENV + 7 * 2:
  case (u32)BPAddress::TEV_COLOR_ENV + 8 * 2:
  case (u32)BPAddress::TEV_COLOR_ENV + 9 * 2:
  case (u32)BPAddress::TEV_COLOR_ENV + 10 * 2:
  case (u32)BPAddress::TEV_COLOR_ENV + 11 * 2:
  case (u32)BPAddress::TEV_COLOR_ENV + 12 * 2:
  case (u32)BPAddress::TEV_COLOR_ENV + 13 * 2:
  case (u32)BPAddress::TEV_COLOR_ENV + 14 * 2:
  case (u32)BPAddress::TEV_COLOR_ENV + 15 * 2:
    mGpuShader.setReg<ColorCombiner>(
        mGpuShader
            .colorEnv[((u32)token.reg - (u32)BPAddress::TEV_COLOR_ENV) / 2],
        token);
    break;
  case (u32)BPAddress::TEV_ALPHA_ENV + 0 * 2:
  case (u32)BPAddress::TEV_ALPHA_ENV + 1 * 2:
  case (u32)BPAddress::TEV_ALPHA_ENV + 2 * 2:
  case (u32)BPAddress::TEV_ALPHA_ENV + 3 * 2:
  case (u32)BPAddress::TEV_ALPHA_ENV + 4 * 2:
  case (u32)BPAddress::TEV_ALPHA_ENV + 5 * 2:
  case (u32)BPAddress::TEV_ALPHA_ENV + 6 * 2:
  case (u32)BPAddress::TEV_ALPHA_ENV + 7 * 2:
  case (u32)BPAddress::TEV_ALPHA_ENV + 8 * 2:
  case (u32)BPAddress::TEV_ALPHA_ENV + 9 * 2:
  case (u32)BPAddress::TEV_ALPHA_ENV + 10 * 2:
  case (u32)BPAddress::TEV_ALPHA_ENV + 11 * 2:
  case (u32)BPAddress::TEV_ALPHA_ENV + 12 * 2:
  case (u32)BPAddress::TEV_ALPHA_ENV + 13 * 2:
  case (u32)BPAddress::TEV_ALPHA_ENV + 14 * 2:
  case (u32)BPAddress::TEV_ALPHA_ENV + 15 * 2:
    mGpuShader.setReg<AlphaCombiner>(
        mGpuShader
            .alphaEnv[((u32)token.reg - (u32)BPAddress::TEV_ALPHA_ENV) / 2],
        token);
    break;
  case (u32)BPAddress::IND_CMD + 0:
  case (u32)BPAddress::IND_CMD + 1:
  case (u32)BPAddress::IND_CMD + 2:
  case (u32)BPAddress::IND_CMD + 3:
  case (u32)BPAddress::IND_CMD + 4:
  case (u32)BPAddress::IND_CMD + 5:
  case (u32)BPAddress::IND_CMD + 6:
  case (u32)BPAddress::IND_CMD + 7:
  case (u32)BPAddress::IND_CMD + 8:
  case (u32)BPAddress::IND_CMD + 9:
  case (u32)BPAddress::IND_CMD + 10:
  case (u32)BPAddress::IND_CMD + 11:
  case (u32)BPAddress::IND_CMD + 12:
  case (u32)BPAddress::IND_CMD + 13:
  case (u32)BPAddress::IND_CMD + 14:
  case (u32)BPAddress::IND_CMD + 15:
    mGpuShader.setReg<TevStageIndirect>(
        mGpuShader.indCmd[(u32)token.reg - (u32)BPAddress::IND_CMD], token);
    mGpuShader.defineIndCmd((u32)token.reg - (u32)BPAddress::IND_CMD);
    break;
  case BPAddress::BP_MASK:
    mGpuShader.mMask = 0xff000000 | (token.val & 0x00ffffff);
    break;
  default:
    return std::unexpected(std::format("Unexpected BP command: 0x{:x}",
                                       static_cast<u32>(token.reg)));
  }
  // If mask has expired, reset it
  if (mGpuShader.mMask != 0xffffffff && token.reg != (u32)BPAddress::BP_MASK)
    mGpuShader.mMask = 0xffffffff;
  return {};
}
Result<void> QDisplayListShaderHandler::onStreamEnd() {
  bool corrupt = false;

  // four swap tables and indirect stages
  for (int i = 0; i < 4; i++) {
    mMaterial.mSwapTable[i].r =
        (gx::ColorComponent)mGpuShader.tevksel[i * 2].swaprb.Value();
    mMaterial.mSwapTable[i].g =
        (gx::ColorComponent)mGpuShader.tevksel[i * 2].swapga.Value();
    mMaterial.mSwapTable[i].b =
        (gx::ColorComponent)mGpuShader.tevksel[i * 2 + 1].swaprb.Value();
    mMaterial.mSwapTable[i].a =
        (gx::ColorComponent)mGpuShader.tevksel[i * 2 + 1].swapga.Value();

    if (i < mMaterial.indirectStages.size()) {
      mMaterial.indirectStages[i].order.refMap = mGpuShader.iref.getTexMap(i);
      mMaterial.indirectStages[i].order.refCoord =
          mGpuShader.iref.getTexCoord(i);
    }
  }

  mMaterial.mStages.resize(mNumStages);

#ifndef NDEBUG
  if (mNumStages % 2) {
    printf("Extra TREF data\n");
    auto tref = mGpuShader.tref[mNumStages / 2];
    auto coord = tref.getTexCoord(1);
    auto chan = tref.getColorChan(1);
    auto enable = tref.getEnable(1);
    auto map = tref.getTexMap(1);

    printf("coord, chan, enable, map: %u, %u, %u, %u\n", coord, chan, enable,
           map);
  }
#endif

  for (int i = 0; i < mNumStages; i++) // evenStageId
  {
    auto& stage = mMaterial.mStages[i];

    // TREF
    stage.texCoord = mGpuShader.tref[i / 2].getTexCoord(i & 1);
    switch ((gx::ColorSelChanLow)mGpuShader.tref[i / 2].getColorChan(i & 1)) {
    case gx::ColorSelChanLow::color0a0:
      stage.rasOrder = gx::ColorSelChanApi::color0a0;
      break;
    case gx::ColorSelChanLow::color1a1:
      stage.rasOrder = gx::ColorSelChanApi::color1a1;
      break;
    case gx::ColorSelChanLow::ind_alpha:
      stage.rasOrder = gx::ColorSelChanApi::ind_alpha;
      break;
    case gx::ColorSelChanLow::normalized_ind_alpha:
      stage.rasOrder = gx::ColorSelChanApi::normalized_ind_alpha;
      break;
    case gx::ColorSelChanLow::null:
      stage.rasOrder = gx::ColorSelChanApi::null;
      break;
    }
    stage.texMap = mGpuShader.tref[i / 2].getEnable(i & 1)
                       ? mGpuShader.tref[i / 2].getTexMap(i & 1)
                       : -1; // TEXMAP_NULL

    // KSEL
    stage.colorStage.constantSelection =
        (gx::TevKColorSel)mGpuShader.tevksel[i / 2].getKC(i & 1);
    stage.alphaStage.constantSelection =
        (gx::TevKAlphaSel)mGpuShader.tevksel[i / 2].getKA(i & 1);

    // COLOR_ENV
    {
      const auto& cEnv = mGpuShader.colorEnv[i];
      auto& dst = stage.colorStage;

      dst.out = (gx::TevReg)cEnv.dest.Value();
      dst.clamp = cEnv.clamp.Value();

      if (cEnv.bias.Value() == 3) {
        dst.formula =
            (gx::TevColorOp)((cEnv.op.Value() | (cEnv.shift.Value() << 1)) +
                             (u32)gx::TevColorOp::comp_r8_gt);
        dst.bias = gx::TevBias::zero;
        dst.scale = gx::TevScale::scale_1;
      } else {
        dst.formula = (gx::TevColorOp)cEnv.op.Value();
        dst.bias = (gx::TevBias)cEnv.bias.Value();
        dst.scale = (gx::TevScale)cEnv.shift.Value();
      }

      dst.a = (gx::TevColorArg)cEnv.a.Value();
      dst.b = (gx::TevColorArg)cEnv.b.Value();
      dst.c = (gx::TevColorArg)cEnv.c.Value();
      dst.d = (gx::TevColorArg)cEnv.d.Value();
    }
    // ALPHA_ENV
    {
      const auto& aEnv = mGpuShader.alphaEnv[i];
      auto& dst = stage.alphaStage;

      dst.out = (gx::TevReg)aEnv.dest.Value();
      dst.clamp = aEnv.clamp.Value();

      if (aEnv.bias.Value() == 3) {
        dst.formula =
            (gx::TevAlphaOp)((aEnv.op.Value() | (aEnv.shift.Value() << 1)) +
                             (u32)gx::TevAlphaOp::comp_r8_gt);
        dst.bias = gx::TevBias::zero;
        dst.scale = gx::TevScale::scale_1;
      } else {
        dst.formula = (gx::TevAlphaOp)aEnv.op.Value();
        dst.bias = (gx::TevBias)aEnv.bias.Value();
        dst.scale = (gx::TevScale)aEnv.shift.Value();
      }

      dst.a = (gx::TevAlphaArg)aEnv.a.Value();
      dst.b = (gx::TevAlphaArg)aEnv.b.Value();
      dst.c = (gx::TevAlphaArg)aEnv.c.Value();
      dst.d = (gx::TevAlphaArg)aEnv.d.Value();

      // Additional: Swap table selection
      stage.texMapSwap = aEnv.tswap.Value();
      stage.rasSwap = aEnv.rswap.Value();
    }
    // IND_CMD
    {
      int j = i;
      if (!mGpuShader.isDefinedIndCmd(i)) {
        j >>= 1;
        corrupt = true;
      }
      const auto& iCmd = mGpuShader.indCmd[j];
      auto& dst = stage.indirectStage;

      dst.indStageSel = iCmd.bt.Value();
      dst.format = (gx::IndTexFormat)iCmd.fmt.Value();
      dst.bias = (gx::IndTexBiasSel)iCmd.bias.Value();
      dst.matrix = (gx::IndTexMtxID)iCmd.mid.Value();
      dst.wrapU = (gx::IndTexWrap)iCmd.sw.Value();
      dst.wrapV = (gx::IndTexWrap)iCmd.tw.Value();
      dst.addPrev = iCmd.fb_addprev.Value();
      dst.utcLod = iCmd.lb_utclod.Value();
      dst.alpha = (gx::IndTexAlphaSel)iCmd.bs.Value();
    }
  }

  if (corrupt) {
    // throw std::runtime_error("Corrupt model data");
  }

  return {};
}

QDisplayListMaterialHandler::QDisplayListMaterialHandler(
    gx::LowLevelGxMaterial& mat)
    : mMat(mat) {}
QDisplayListMaterialHandler::~QDisplayListMaterialHandler() = default;
enum RegType { TEV_COLOR_REG = 0, TEV_KONSTANT_REG = 1 };
Result<void> QDisplayListMaterialHandler::onCommandXF(const QXFCommand& token) {
  switch (token.reg) {
  case XF_TEX0_ID:
  case XF_TEX0_ID + 1:
  case XF_TEX0_ID + 2:
  case XF_TEX0_ID + 3:
  case XF_TEX0_ID + 4:
  case XF_TEX0_ID + 5:
  case XF_TEX0_ID + 6:
  case XF_TEX0_ID + 7:
    mGpuMat.mTexture[token.reg - XF_TEX0_ID].tex.hex = token.val;
    return {};
  case XF_DUALTEX0_ID:
  case XF_DUALTEX0_ID + 1:
  case XF_DUALTEX0_ID + 2:
  case XF_DUALTEX0_ID + 3:
  case XF_DUALTEX0_ID + 4:
  case XF_DUALTEX0_ID + 5:
  case XF_DUALTEX0_ID + 6:
  case XF_DUALTEX0_ID + 7:
    mGpuMat.mTexture[token.reg - XF_DUALTEX0_ID].dualTex.hex = token.val;
    return {};
  }

  return std::unexpected(std::format("Unexpected XF command: 0x{:x}",
                                     static_cast<u32>(token.reg)));
}
Result<void> QDisplayListMaterialHandler::onCommandBP(const QBPCommand& token) {
  switch ((u32)token.reg) {
  case BPAddress::ALPHACOMPARE:
    mGpuMat.setReg(mGpuMat.mPixel.mAlphaCompare, token);
    break;
  case BPAddress::ZMODE:
    mGpuMat.setReg(mGpuMat.mPixel.mZMode, token);
    break;
  case BPAddress::BP_MASK:
    mGpuMat.mMask = 0xff000000 | (token.val & 0x00ffffff);
    break;
  case BPAddress::BLENDMODE:
    mGpuMat.setReg(mGpuMat.mPixel.mBlendMode, token);
    break;
  case BPAddress::CONSTANTALPHA:
    mGpuMat.setReg(mGpuMat.mPixel.mDstAlpha, token);
    break;
  case BPAddress::TEV_COLOR_RA:
  case BPAddress::TEV_COLOR_RA + 2:
  case BPAddress::TEV_COLOR_RA + 4:
  case BPAddress::TEV_COLOR_RA + 6: {
    GPUTevReg tmp;
    tmp.low = (token.val & mGpuMat.mMask);
    if (tmp.type_ra.Value() == TEV_KONSTANT_REG) {
      auto& gpuReg =
          mGpuMat.mShaderColor
              .Konstants[((u32)token.reg - (u32)BPAddress::TEV_COLOR_RA) / 2];
      gpuReg.low = (gpuReg.low.Value() & ~static_cast<u64>(mGpuMat.mMask)) |
                   (token.val & static_cast<u64>(mGpuMat.mMask));
    } else {
      auto& gpuReg =
          mGpuMat.mShaderColor
              .Registers[((u32)token.reg - (u32)BPAddress::TEV_COLOR_RA) / 2];
      gpuReg.low = (gpuReg.low.Value() & ~static_cast<u64>(mGpuMat.mMask)) |
                   (token.val & static_cast<u64>(mGpuMat.mMask));
    }
    break;
  }
  case BPAddress::TEV_COLOR_BG:
  case BPAddress::TEV_COLOR_BG + 2:
  case BPAddress::TEV_COLOR_BG + 4:
  case BPAddress::TEV_COLOR_BG + 6: {
    GPUTevReg tmp;
    tmp.high = (token.val & mGpuMat.mMask);
    if (tmp.type_bg.Value() == TEV_KONSTANT_REG) {
      auto& gpuReg =
          mGpuMat.mShaderColor
              .Konstants[((u32)token.reg - (u32)BPAddress::TEV_COLOR_RA) / 2];
      gpuReg.high = (gpuReg.high.Value() & ~static_cast<u64>(mGpuMat.mMask)) |
                    (token.val & static_cast<u64>(mGpuMat.mMask));
    } else {
      auto& gpuReg =
          mGpuMat.mShaderColor
              .Registers[((u32)token.reg - (u32)BPAddress::TEV_COLOR_RA) / 2];
      gpuReg.high = (gpuReg.high.Value() & ~static_cast<u64>(mGpuMat.mMask)) |
                    (token.val & static_cast<u64>(mGpuMat.mMask));
    }
    break;
  }
  case BPAddress::RAS1_SS0:
  case BPAddress::RAS1_SS0 + 1:
    mGpuMat.setReg(mGpuMat.mIndirect.mIndTexScales[(u32)token.reg -
                                                   (u32)BPAddress::RAS1_SS0],
                   token);
    break;
  case BPAddress::IND_MTXA:
  case BPAddress::IND_MTXA + 3:
  case BPAddress::IND_MTXA + 6:
    mGpuMat.setReg(
        mGpuMat.mIndirect
            .mIndMatrices[((u32)token.reg - (u32)BPAddress::IND_MTXA) / 3]
            .col0,
        token);
    break;
  case BPAddress::IND_MTXB:
  case BPAddress::IND_MTXB + 3:
  case BPAddress::IND_MTXB + 6:
    mGpuMat.setReg(
        mGpuMat.mIndirect
            .mIndMatrices[((u32)token.reg - (u32)BPAddress::IND_MTXA) / 3]
            .col1,
        token);
    break;
  case BPAddress::IND_MTXC:
  case BPAddress::IND_MTXC + 3:
  case BPAddress::IND_MTXC + 6:
    mGpuMat.setReg(
        mGpuMat.mIndirect
            .mIndMatrices[((u32)token.reg - (u32)BPAddress::IND_MTXA) / 3]
            .col2,
        token);
    break;
  default: {
    return std::unexpected(std::format("Unexpected BP command: 0x{:x}",
                                       static_cast<u32>(token.reg)));
  }
  }
  // If mask has expired, reset it
  if (mGpuMat.mMask != 0xffffffff && token.reg != (u32)BPAddress::BP_MASK)
    mGpuMat.mMask = 0xffffffff;
  return {};
}
Result<void>
QDisplayListVertexSetupHandler::onCommandBP(const QBPCommand& token) {
  // TODO: Ignores cull mode repeat
  return {};
}
Result<void>
QDisplayListVertexSetupHandler::onCommandCP(const QCPCommand& token) {
  switch (token.reg) {
  case 0x50:
    mGpuMesh.VCD.Hex = token.val;
    return {};
  case 0x60:
    mGpuMesh.VCD.Hex |= (token.val << 17);
    return {};
  case 0x70:
  case 0x80:
  case 0x90:
    // TODO: we encounter these but simply ignore them.
    return {};
  // Runtime only typically, but CTools seems to set it
  case 0xa0:
  case 0xa1:
  case 0xb0:
  case 0xb1:
    return {};
  }
  auto s = std::format("Unexpected command in display list: 0x{:x}", token.reg);
  printf("%s\n", s.c_str());
  return {};
}
Result<void>
QDisplayListVertexSetupHandler::onCommandXF(const QXFCommand& token) {
  // TODO: Validate XF_INVTXSPEC_ID
  return {};
}

} // namespace librii::gpu
