#pragma once

#include <array>
#include <core/common.h>
#include <functional>
#include <librii/gpu/GPUAddressSpace.hpp>
#include <librii/gpu/GPUMaterial.hpp>
#include <oishii/writer/binary_writer.hxx>
#include <span>

namespace librii::gpu {

class DLBuilder {
public:
  DLBuilder(oishii::Writer& writer) : mWriter(writer) {}
  ~DLBuilder() = default;

  struct TexDelegate {
    TexDelegate(DLBuilder& builder, u8 id) : mBuilder(builder), mId(id) {}

    void setImagePointer(u32 physical_addr, bool use_raw = false) {
      assert(!use_raw || physical_addr % 32 == 0 &&
                             "Physical address has too must granularity "
                             "(restricted to 32-byte cache lines).");
      std::array<u8, 8> lut{0x94, 0x95, 0x96, 0x97, 0xB4, 0xB5, 0xB6, 0xB7};

      mBuilder.writeBp(lut[mId], use_raw ? physical_addr : physical_addr << 5);
    }
    void setImageAttributes(u16 width, u16 height,
                            librii::gx::TextureFormat format) {
      assert(width <= 1024 && height <= 1024 &&
             "No dimension may exceed 1024x1024");
      std::array<u8, 8> lut{0x94, 0x95, 0x96, 0x97, 0xB4, 0xB5, 0xB6, 0xB7};

      constexpr u32 dimension_mask = (1 << 10) - 1;
      constexpr u32 format_mask = (1 << 4) - 1;

      const u32 width_field = (width - 1) & dimension_mask;
      const u32 height_field = (height - 1) & dimension_mask;
      const u32 format_field = static_cast<u32>(format) & format_mask;

      mBuilder.writeBp(lut[mId], width_field | (height_field << 10) |
                                     (format_field << 20));
    }
    void setLookupMode(librii::gx::TextureWrapMode wrap_s,
                       librii::gx::TextureWrapMode wrap_t,
                       librii::gx::TextureFilter min_filt,
                       librii::gx::TextureFilter mag_filt, f32 min_lod,
                       f32 max_lod, f32 lod_bias, bool clamp_bias,
                       bool edge_lod, librii::gx::AnisotropyLevel aniso) {
      std::array<u8, 8> lut0{0x80, 0x81, 0x82, 0x83, 0xA0, 0xA1, 0xA2, 0xA3};
      std::array<u8, 8> lut1{0x84, 0x85, 0x86, 0x87, 0xA4, 0xA5, 0xA6, 0xA7};

      std::array<u8, 6> filter_lut{0, 4, 1, 5, 2, 6};

      librii::gpu::TexMode0 mode0;
      mode0.hex = 0;
      librii::gpu::TexMode1 mode1;
      mode1.hex = 0;

      mode0.wrap_s = static_cast<u32>(wrap_s);
      mode0.wrap_t = static_cast<u32>(wrap_t);
      mode0.mag_filter = mag_filt == librii::gx::TextureFilter::Linear;
      mode0.min_filter = filter_lut[static_cast<u32>(mag_filt)];
      mode0.diag_lod = !edge_lod;
      mode0.lod_bias =
          static_cast<u32>(roundf(32.0f * static_cast<f32>(lod_bias)));
      mode0.max_aniso = static_cast<u32>(aniso);
      mode0.lod_clamp = clamp_bias;

      mode1.min_lod =
          static_cast<u32>(roundf(static_cast<f32>(min_lod) * 16.0f));
      mode1.max_lod =
          static_cast<u32>(roundf(static_cast<f32>(max_lod) * 16.0f));

      mBuilder.writeBp(lut0[mId], mode0.hex);
      mBuilder.writeBp(lut1[mId], mode1.hex);
    }

    void setTLUT() {}

  private:
    DLBuilder& mBuilder;
    u32 mId;
  };
  TexDelegate setTexture(u8 id) {
    assert(id < 8 && "Only 8 textures may be supplied.");
    return {*this, id};
  }
  void setTexCoordGen(u8 id, librii::gx::TexCoordGen gen) {
    using namespace librii::gx;
    assert(id < 8 && "Only 8 texture coordinate channels may be supplied");

    gpu::XF_TEXTURE xf_tex;
    xf_tex.tex.hex = 0;
    xf_tex.dualTex.hex = 0;

    xf_tex.tex.projection = gpu::XF_TEX_ST;
    xf_tex.tex.inputform = gpu::XF_TEX_AB11;
    xf_tex.tex.texgentype = gpu::XF_TEXGEN_REGULAR;
    xf_tex.tex.sourcerow = gpu::XF_TEX0_INROW;
    xf_tex.tex.embosssourceshift = gpu::XF_TEX0_INROW;
    xf_tex.tex.embosslightshift = 0;

    xf_tex.dualTex.index = static_cast<u32>(gen.postMatrix) -
                           static_cast<u32>(PostTexMatrix::Matrix0);
    xf_tex.dualTex.normalize = gen.normalize;

    switch (gen.sourceParam) {
    case TexGenSrc::Position:
      xf_tex.tex.sourcerow = gpu::XF_GEOM_INROW;
      xf_tex.tex.inputform = gpu::XF_TEX_ABC1;
      break;
    case TexGenSrc::Normal:
      xf_tex.tex.sourcerow = gpu::XF_NORMAL_INROW;
      xf_tex.tex.inputform = gpu::XF_TEX_ABC1;
      break;
    case TexGenSrc::Binormal:
      xf_tex.tex.sourcerow = gpu::XF_BINORMAL_T_INROW;
      xf_tex.tex.inputform = gpu::XF_TEX_ABC1;
      break;
    case TexGenSrc::Tangent:
      xf_tex.tex.sourcerow = gpu::XF_BINORMAL_B_INROW;
      xf_tex.tex.inputform = gpu::XF_TEX_ABC1;
      break;
    case TexGenSrc::Color0:
    case TexGenSrc::Color1:
      xf_tex.tex.sourcerow = gpu::XF_COLORS_INROW;
      break;
    case TexGenSrc::UV0:
    case TexGenSrc::UV1:
    case TexGenSrc::UV2:
    case TexGenSrc::UV3:
    case TexGenSrc::UV4:
    case TexGenSrc::UV5:
    case TexGenSrc::UV6:
    case TexGenSrc::UV7:
      xf_tex.tex.sourcerow = gpu::XF_TEX0_INROW +
                             static_cast<u32>(gen.sourceParam) -
                             static_cast<u32>(TexGenSrc::UV0);
      break;
    case TexGenSrc::BumpUV0:
    case TexGenSrc::BumpUV1:
    case TexGenSrc::BumpUV2:
    case TexGenSrc::BumpUV3:
    case TexGenSrc::BumpUV4:
    case TexGenSrc::BumpUV5:
    case TexGenSrc::BumpUV6:
      xf_tex.tex.embosssourceshift = static_cast<u32>(gen.sourceParam) -
                                     static_cast<u32>(TexGenSrc::BumpUV0);
      break;
    default:
      assert(!"Invalid source param");
      break;
    }

    switch (gen.func) {
    case TexGenType::Matrix2x4:
      xf_tex.tex.texgentype = gpu::XF_TEXGEN_REGULAR;
      break;
    case TexGenType::Matrix3x4:
      xf_tex.tex.texgentype = gpu::XF_TEXGEN_REGULAR;
      xf_tex.tex.projection = gpu::XF_TEX_STQ;
      break;
    case TexGenType::Bump0:
    case TexGenType::Bump1:
    case TexGenType::Bump2:
    case TexGenType::Bump3:
    case TexGenType::Bump4:
    case TexGenType::Bump5:
    case TexGenType::Bump6:
    case TexGenType::Bump7:
      assert(static_cast<u32>(gen.sourceParam) >=
                 static_cast<u32>(TexGenSrc::BumpUV0) &&
             static_cast<u32>(gen.sourceParam) <=
                 static_cast<u32>(TexGenSrc::BumpUV6) &&
             "Invalid coordinate source");
      xf_tex.tex.texgentype = gpu::XF_TEXGEN_EMBOSS_MAP;
      xf_tex.tex.embosslightshift =
          static_cast<u32>(gen.func) - static_cast<u32>(TexGenType::Bump0);
      break;
    case TexGenType::SRTG:
      xf_tex.tex.texgentype = gen.sourceParam == TexGenSrc::Color0
                                  ? gpu::XF_TEXGEN_COLOR_STRGBC0
                                  : gpu::XF_TEXGEN_COLOR_STRGBC1;
      break;
    default:
      assert(!"Invalid function");
      break;
    }

    writeXf(gpu::XF_TEX0_ID + id, xf_tex.tex.hex);
    writeXf(gpu::XF_DUALTEX0_ID + id, xf_tex.dualTex.hex);
  }
  void loadTLUT() {}

  void setTevKonstantSel(u8 even_id, librii::gx::TevKColorSel kc0,
                         librii::gx::TevKAlphaSel ka0,
                         librii::gx::TevKColorSel kc1,
                         librii::gx::TevKAlphaSel ka1) {
    assert(even_id <= 16 && even_id % 2 == 0 && "Invalid stage id");
    bpMask(0xfffff0);
    librii::gpu::TevKSel sel;
    sel.hex = 0;
    sel.kcsel0 = static_cast<u32>(kc0);
    sel.kasel0 = static_cast<u32>(ka0);
    sel.kcsel1 = static_cast<u32>(kc1);
    sel.kasel1 = static_cast<u32>(ka1);
    writeBp(BPAddress::TEV_KSEL + even_id / 2, sel.hex);
  }
  void setTevOrder(u8 even_id, u8 gen0, u8 tex0,
                   librii::gx::ColorSelChanApi chan0, u8 gen1, u8 tex1,
                   librii::gx::ColorSelChanApi chan1) {
    using namespace librii::gx;
    assert(even_id <= 16 && even_id % 2 == 0 && "Invalid stage id");
    constexpr ColorSelChanLow cvt[] = {ColorSelChanLow::color0a0,
                                       ColorSelChanLow::color1a1,
                                       ColorSelChanLow::color0a0,
                                       ColorSelChanLow::color1a1,
                                       ColorSelChanLow::color0a0,
                                       ColorSelChanLow::color1a1,
                                       ColorSelChanLow::null,
                                       ColorSelChanLow::ind_alpha,
                                       ColorSelChanLow::normalized_ind_alpha,
                                       ColorSelChanLow::color0a0,
                                       ColorSelChanLow::color0a0,
                                       ColorSelChanLow::color0a0,
                                       ColorSelChanLow::color0a0,
                                       ColorSelChanLow::color0a0,
                                       ColorSelChanLow::color0a0,
                                       ColorSelChanLow::null};
    librii::gpu::RAS1_TREF tref;
    tref.hex = 0;
    tref.texmap0 = tex0;
    tref.texcoord0 = gen0;
    tref.enable0 = tex0 != 0xff; // no enable mask
    tref.colorchan0 = chan0 != ColorSelChanApi::null
                          ? static_cast<u32>(cvt[(u32)chan0])
                          : (u32)ColorSelChanLow::null;

    tref.texmap1 = tex1;
    tref.texcoord1 = gen1;
    tref.enable1 = tex1 != 0xff; // no enable mask
    tref.colorchan1 = chan1 != ColorSelChanApi::null
                          ? static_cast<u32>(cvt[(u32)chan1])
                          : (u32)ColorSelChanLow::null;

    writeBp(BPAddress::TREF + even_id / 2, tref.hex);
  }
  void setTevOrder(u8 even_id, librii::gx::TevStage even,
                   librii::gx::TevStage odd) {
    setTevOrder(even_id, even.texCoord, even.texMap, even.rasOrder,
                odd.texCoord, odd.texMap, odd.rasOrder);
  }
  void setTexCoordScale2() {}

  // 0 is tevprev
  void setTevColor(u8 reg, librii::gx::ColorS10 color,
                   u32 type = 0 /* color reg*/) {
    librii::gpu::GPUTevReg tevreg;
    tevreg.hex = 0;
    tevreg.red = color.r;
    tevreg.alpha = color.a;
    tevreg.type_ra = type;
    tevreg.blue = color.b;
    tevreg.green = color.g;
    tevreg.type_bg = type;

    const u32 ra = tevreg.low.Value();
    const u32 bg = tevreg.high.Value();

    writeBp(BPAddress::TEV_REGISTERL_0_ID + reg * 2, ra);
    writeBp(BPAddress::TEV_REGISTERH_0_ID + reg * 2, bg);

    // KCOLOR does not have a timing bug
    if (type == 0) {
      writeBp(BPAddress::TEV_REGISTERH_0_ID + reg * 2, bg);
      writeBp(BPAddress::TEV_REGISTERH_0_ID + reg * 2, bg);
    }
  }
  void setTevKColor(u8 reg, librii::gx::Color color) {
    setTevColor(reg,
                librii::gx::ColorS10{
                    static_cast<s32>(color.r), static_cast<s32>(color.g),
                    static_cast<s32>(color.b), static_cast<s32>(color.a)},
                1 /* konst reg*/);
  }
  void setTevColorCalc(u8 id, librii::gx::TevColorArg a,
                       librii::gx::TevColorArg b, librii::gx::TevColorArg c,
                       librii::gx::TevColorArg d, librii::gx::TevColorOp op,
                       librii::gx::TevBias bias, librii::gx::TevScale scale,
                       bool clamp, librii::gx::TevReg dst) {
    librii::gpu::ColorCombiner color_env;
    color_env.hex = 0;

    color_env.d = static_cast<u32>(d);
    color_env.c = static_cast<u32>(c);
    color_env.b = static_cast<u32>(b);
    color_env.a = static_cast<u32>(a);

    if ((u32)op <= (u32)librii::gx::TevColorOp::subtract) {
      color_env.bias = static_cast<u32>(bias);
      color_env.op = static_cast<u32>(op) & 1;
      color_env.clamp = clamp;
      color_env.shift = static_cast<u32>(scale);
      color_env.dest = static_cast<u32>(dst);
    } else {
      color_env.bias = 3;
      color_env.op = static_cast<u32>(op) & 1;
      color_env.clamp = clamp;
      color_env.shift = (static_cast<u32>(op) >> 1) & 3;
      color_env.dest = static_cast<u32>(dst);
    }

    writeBp(BPAddress::TEV_COLOR_ENV + id * 2, color_env.hex);
  }
  void setTevColorCalc(u8 id, librii::gx::TevStage::ColorStage stage) {
    setTevColorCalc(id, stage.a, stage.b, stage.c, stage.d, stage.formula,
                    stage.bias, stage.scale, stage.clamp, stage.out);
  }
  void
  setTevAlphaCalcAndSwap(u8 id, librii::gx::TevAlphaArg a,
                         librii::gx::TevAlphaArg b, librii::gx::TevAlphaArg c,
                         librii::gx::TevAlphaArg d, librii::gx::TevAlphaOp op,
                         librii::gx::TevBias bias, librii::gx::TevScale scale,
                         bool clamp, librii::gx::TevReg dst, u8 ras_swap,
                         u8 tex_swap) {
    librii::gpu::AlphaCombiner alpha_env;
    alpha_env.hex = 0;

    alpha_env.rswap = ras_swap;
    alpha_env.tswap = tex_swap;

    alpha_env.d = static_cast<u32>(d);
    alpha_env.c = static_cast<u32>(c);
    alpha_env.b = static_cast<u32>(b);
    alpha_env.a = static_cast<u32>(a);

    if ((u32)op <= (u32)librii::gx::TevAlphaOp::subtract) {
      alpha_env.bias = static_cast<u32>(bias);
      alpha_env.op = static_cast<u32>(op) & 1;
      alpha_env.clamp = clamp;
      alpha_env.shift = static_cast<u32>(scale);
      alpha_env.dest = static_cast<u32>(dst);
    } else {
      alpha_env.bias = 3;
      alpha_env.op = static_cast<u32>(op) & 1;
      alpha_env.clamp = clamp;
      alpha_env.shift = (static_cast<u32>(op) >> 1) & 3;
      alpha_env.dest = static_cast<u32>(dst);
    }

    writeBp(BPAddress::TEV_ALPHA_ENV + id * 2, alpha_env.hex);
  }
  void setTevAlphaCalcAndSwap(u8 id, librii::gx::TevStage::AlphaStage stage,
                              u8 ras_swap, u8 tex_swap) {
    setTevAlphaCalcAndSwap(id, stage.a, stage.b, stage.c, stage.d,
                           stage.formula, stage.bias, stage.scale, stage.clamp,
                           stage.out, ras_swap, tex_swap);
  }
  void setTevIndirect(u8 id, librii::gx::TevStage::IndirectStage stage) {
    assert(id < 16 && "Invalid stage id");
    gpu::TevStageIndirect reg;
    reg.bt = stage.indStageSel;
    reg.fmt = static_cast<u32>(stage.format);
    reg.bias = static_cast<u32>(stage.bias);
    reg.bs = static_cast<u32>(stage.alpha);
    reg.mid = static_cast<u32>(stage.matrix);
    reg.sw = static_cast<u32>(stage.wrapU);
    reg.tw = static_cast<u32>(stage.wrapV);
    reg.lb_utclod = stage.utcLod;
    reg.fb_addprev = stage.addPrev;
    writeBp(BPAddress::IND_CMD + id, reg.hex);
  }
  void setTevSwapModeTable(u8 id, librii::gx::SwapTableEntry swap) {
    assert(id <= 3 && "Invalid swap table index");
    librii::gpu::TevKSel ksel;
    ksel.hex = 0;
    ksel.swaprb = static_cast<u32>(swap.r);
    ksel.swapga = static_cast<u32>(swap.g);
    bpMask(0xf);
    writeBp(BPAddress::TEV_KSEL + (id * 2), ksel.hex);
    ksel.hex = 0;
    ksel.swaprb = static_cast<u32>(swap.b);
    ksel.swapga = static_cast<u32>(swap.a);
    bpMask(0xf);
    writeBp(BPAddress::TEV_KSEL + (id * 2) + 1, ksel.hex);
  }
  void setTevKonstantSelAndSwapModeTable() {}
  void setIndTexMtx(u8 id, const librii::gx::IndirectMatrix& mtx) {
    // indirect matrix is column-major, same as glm
    setIndTexMtx(id, mtx.compute());
  }
  // column-major
  void setIndTexMtx(u8 id, const glm::mat3x2& mtx) {
    assert(id <= 2 && "Invalid indirect texture matrix index");

    // COLUMN MAJOR
    glm::mat3x2 m = mtx;
    glm::mat3x2 a = mtx;

    for (int c = 0; c < 3; ++c)
      for (int r = 0; r < 2; ++r)
        a[c][r] = std::abs(a[c][r]);

    s8 mantissa = 0;

    const auto bring_into_range = [&]() {
      const auto any_of = [&](auto comp, auto ref) {
        return comp(a[0][0], ref) || comp(a[0][1], ref) || comp(a[1][0], ref) ||
               comp(a[1][1], ref) || comp(a[2][0], ref) || comp(a[2][1], ref);
      };
      const auto all_of = [&](auto comp, auto ref) {
        return comp(a[0][0], ref) && comp(a[0][1], ref) && comp(a[1][0], ref) &&
               comp(a[1][1], ref) && comp(a[2][0], ref) && comp(a[2][1], ref);
      };
      // If we're out of range, increase the scale exponent and decrease each
      // element to bring us into range.
      bool above_range = false;
      while (any_of(std::greater_equal<f32>(), 1.0f) && mantissa < 46) {
        above_range = true;
        ++mantissa;

        m *= 0.5f;
        a *= 0.5f;
      }
      // We know we can't go up
      if (above_range)
        return;

      while (all_of(std::less<f32>(), 0.5f) && mantissa > -17) {
        --mantissa;

        m *= 2.0f;
        a *= 2.0f;
      }
    };
    bring_into_range();
    mantissa += 0x11; // bias

    const auto elem = [](f32 x) -> f32 {
      return static_cast<s32>(x * (1 << 10)) & 0x7ff;
    };

    librii::gpu::IND_MTX gpu_mtx;
    gpu_mtx.col0.ma = elem(m[0][0]);
    gpu_mtx.col0.mb = elem(m[0][1]);
    gpu_mtx.col0.s0 = mantissa & 0x03;

    gpu_mtx.col1.mc = elem(m[1][0]);
    gpu_mtx.col1.md = elem(m[1][1]);
    gpu_mtx.col1.s1 = (mantissa >> 2) & 0x03;

    gpu_mtx.col2.me = elem(m[2][0]);
    gpu_mtx.col2.mf = elem(m[2][1]);
    gpu_mtx.col2.s2 = (mantissa >> 4) & 0x03;

    writeBp(BPAddress::IND_MTXA + (id * 3), gpu_mtx.col0.hex);
    writeBp(BPAddress::IND_MTXB + (id * 3), gpu_mtx.col1.hex);
    writeBp(BPAddress::IND_MTXC + (id * 3), gpu_mtx.col2.hex);
  }
  void setIndTexCoordScale(u8 evenIndStage,
                           librii::gx::IndirectTextureScalePair s0,
                           librii::gx::IndirectTextureScalePair s1) {
    assert(evenIndStage % 2 == 0 && evenIndStage <= 2 && "Invalid stage index");
    librii::gpu::ras1_ss ras1ss;
    ras1ss.hex = 0;
    ras1ss.ss0 = static_cast<u32>(s0.U);
    ras1ss.ts0 = static_cast<u32>(s0.V);
    ras1ss.ss1 = static_cast<u32>(s1.U);
    ras1ss.ts1 = static_cast<u32>(s1.V);
    writeBp(BPAddress::RAS1_SS0 + evenIndStage / 2, ras1ss.hex);
  }
  void setIndTexOrder(u8 coord0, u8 map0, u8 coord1, u8 map1, u8 coord2,
                      u8 map2, u8 coord3, u8 map3) {
    librii::gpu::RAS1_IREF iref;
    iref.bi0 = map0;
    iref.bc0 = coord0;
    iref.bi1 = map1;
    iref.bc1 = coord1;
    iref.bi2 = map2;
    iref.bc2 = coord2;
    iref.bi3 = map3;
    iref.bc3 = coord3;
    writeBp(BPAddress::IREF, iref.hex);
  }
  void setFog() {}
  void setFogRangeAdj() {}
  void setAlphaCompare(librii::gx::AlphaComparison in) {
    librii::gpu::AlphaTest test;
    test.ref0 = in.refLeft;
    test.ref1 = in.refRight;
    test.comp0 = static_cast<librii::gpu::AlphaTest::CompareMode>(in.compLeft);
    test.comp1 = static_cast<librii::gpu::AlphaTest::CompareMode>(in.compRight);
    test.logic = static_cast<librii::gpu::AlphaTest::Op>(in.op);
    writeBp(BPAddress::ALPHACOMPARE, test.hex);
  }
  void setBlendMode(librii::gx::BlendMode in) {
    bpMask(0xffe3);
    librii::gpu::CMODE0 cmode0;

    cmode0.blendenable =
        static_cast<u32>(in.type == librii::gx::BlendModeType::blend ||
                         in.type == librii::gx::BlendModeType::subtract);
    cmode0.logicopenable =
        static_cast<u32>(in.type == librii::gx::BlendModeType::logic);
    // Masked
    cmode0.dither = 0;
    cmode0.colorupdate = 0;
    cmode0.alphaupdate = 0;
    // ---
    cmode0.dstfactor = static_cast<librii::gpu::CMODE0::BlendFactor>(in.dest);
    cmode0.srcfactor = static_cast<librii::gpu::CMODE0::BlendFactor>(in.source);
    cmode0.subtract = in.type == librii::gx::BlendModeType::subtract;
    cmode0.logicmode = static_cast<librii::gpu::CMODE0::LogicOp>(in.logic);

    writeBp(BPAddress::BLENDMODE, cmode0.hex);
  }
  void setColorAlphaMask(bool colorupdate, bool alphaupdate) {
    const u32 colorupdate_shift = 3;
    const u32 alphaupdate_shift = 4;
    const u32 coloralphaupdate_mask =
        (1 << colorupdate_shift) | (1 << alphaupdate_shift);
    bpMask(coloralphaupdate_mask);

    librii::gpu::CMODE0 cmode0;
    cmode0.colorupdate = colorupdate;
    cmode0.alphaupdate = alphaupdate;
    writeBp(BPAddress::BLENDMODE, cmode0.hex);
  }
  void setZMode(librii::gx::ZMode in) {
    librii::gpu::ZMode zmode;
    zmode.testenable = in.compare;
    zmode.func = static_cast<librii::gpu::ZMode::CompareMode>(in.function);
    zmode.updateenable = in.update;
    writeBp(BPAddress::ZMODE, zmode.hex);
  }
  void setDstAlpha(bool enable, u8 alpha) {
    librii::gpu::CMODE1 cmode1;
    cmode1.alpha = alpha;
    cmode1.enable = enable;
    writeBp(BPAddress::CONSTANTALPHA, cmode1.hex);
  }
  void setChanColor() {}
  void setAmbColor() {}

  // Mesh data
  void setCullMode(librii::gx::CullMode mode) {
    bpMask(3 << 14); // reject
    constexpr std::array<gpu::GenMode::CullMode, 4> cvt{
        gpu::GenMode::CULL_NONE, gpu::GenMode::CULL_FRONT,
        gpu::GenMode::CULL_BACK, gpu::GenMode::CULL_ALL};
    gpu::GenMode gen;
    gen.hex = 0;
    gen.cullmode = cvt[static_cast<int>(mode)];
    writeBp(BPAddress::GENMODE, gen.hex);
  }
  // vtxdesc_low, vtxdesc_high, vtx_spec
  static std::tuple<u32, u32, u32>
  calcVtxDescv(const librii::gx::VertexDescriptor& desc) {
    using namespace librii::gx;
    gpu::TVtxDesc vtx_desc;
    vtx_desc.Hex = 0;
    vtx_desc.Position = static_cast<u32>(VertexAttributeType::Direct);

    u32 normals_count = 0, colors_count = 0, texcoords_count = 0;

    for (auto& [attr, type] : desc.mAttributes) {
      assert((u32)attr < (u32)VertexAttribute::PositionNormalMatrixIndex ||
             (u32)attr > (u32)VertexAttribute::Texture7MatrixIndex ||
             type == VertexAttributeType::None ||
             type == VertexAttributeType::Direct);

      switch (attr) {
      case VertexAttribute::PositionNormalMatrixIndex:
        vtx_desc.PosMatIdx = static_cast<u32>(type);
        break;
      case VertexAttribute::Texture0MatrixIndex:
        vtx_desc.Tex0MatIdx = type != VertexAttributeType::None;
        break;
      case VertexAttribute::Texture1MatrixIndex:
        vtx_desc.Tex1MatIdx = type != VertexAttributeType::None;
        break;
      case VertexAttribute::Texture2MatrixIndex:
        vtx_desc.Tex2MatIdx = type != VertexAttributeType::None;
        break;
      case VertexAttribute::Texture3MatrixIndex:
        vtx_desc.Tex3MatIdx = type != VertexAttributeType::None;
        break;
      case VertexAttribute::Texture4MatrixIndex:
        vtx_desc.Tex4MatIdx = type != VertexAttributeType::None;
        break;
      case VertexAttribute::Texture5MatrixIndex:
        vtx_desc.Tex5MatIdx = type != VertexAttributeType::None;
        break;
      case VertexAttribute::Texture6MatrixIndex:
        vtx_desc.Tex6MatIdx = type != VertexAttributeType::None;
        break;
      case VertexAttribute::Texture7MatrixIndex:
        vtx_desc.Tex7MatIdx = type != VertexAttributeType::None;
        break;
      case VertexAttribute::Position:
        vtx_desc.Position = static_cast<u32>(type);
        break;
      case VertexAttribute::Normal:
        if (type != VertexAttributeType::None) {
          vtx_desc.Normal = static_cast<u32>(type);
          normals_count = 1;
        }
        break;
      case VertexAttribute::NormalBinormalTangent:
        if (type != VertexAttributeType::None) {
          vtx_desc.Normal = static_cast<u32>(type);
          normals_count = 2;
        }
        break;
      case VertexAttribute::Color0:
        vtx_desc.Color0 = static_cast<u32>(type);
        if (type != VertexAttributeType::None)
          ++colors_count;
        break;
      case VertexAttribute::Color1:
        vtx_desc.Color1 = static_cast<u32>(type);
        if (type != VertexAttributeType::None)
          ++colors_count;
        break;
      case VertexAttribute::TexCoord0:
        vtx_desc.Tex0Coord = static_cast<u32>(type);
        if (type != VertexAttributeType::None)
          ++texcoords_count;
        break;
      case VertexAttribute::TexCoord1:
        vtx_desc.Tex1Coord = static_cast<u32>(type);
        if (type != VertexAttributeType::None)
          ++texcoords_count;
        break;
      case VertexAttribute::TexCoord2:
        vtx_desc.Tex2Coord = static_cast<u32>(type);
        if (type != VertexAttributeType::None)
          ++texcoords_count;
        break;
      case VertexAttribute::TexCoord3:
        vtx_desc.Tex3Coord = static_cast<u32>(type);
        if (type != VertexAttributeType::None)
          ++texcoords_count;
        break;
      case VertexAttribute::TexCoord4:
        vtx_desc.Tex4Coord = static_cast<u32>(type);
        if (type != VertexAttributeType::None)
          ++texcoords_count;
        break;
      case VertexAttribute::TexCoord5:
        vtx_desc.Tex5Coord = static_cast<u32>(type);
        if (type != VertexAttributeType::None)
          ++texcoords_count;
        break;
      case VertexAttribute::TexCoord6:
        vtx_desc.Tex6Coord = static_cast<u32>(type);
        if (type != VertexAttributeType::None)
          ++texcoords_count;
        break;
      case VertexAttribute::TexCoord7:
        vtx_desc.Tex7Coord = static_cast<u32>(type);
        if (type != VertexAttributeType::None)
          ++texcoords_count;
        break;
      default:
        break;
      }
    }

    const u32 vd_low = vtx_desc.Hex & ((1 << 17) - 1);
    const u32 vd_high = vtx_desc.Hex >> 17;
    const u32 vspec =
        colors_count | (normals_count << 2) | (texcoords_count << 4);
    return {vd_low, vd_high, vspec};
  }
  void setVtxDescv(const librii::gx::VertexDescriptor& desc) {
    const auto [vd_low, vd_high, vspec] = calcVtxDescv(desc);

    writeCp(0x50, vd_low);
    writeCp(0x60, vd_high);
    writeXf(gpu::XF_INVTXSPEC_ID, vspec);
  }
  void setVtxAttrFmtv(u8 fmtIdx,
                      std::span<std::pair<librii::gx::VertexAttribute,
                                          librii::gx::VQuantization>>
                          quants) {
    assert(fmtIdx < 8);
    using namespace librii::gx;

    gpu::UVAT_group0 group0;
    group0.Hex = 0;
    group0.ByteDequant = 1;
    gpu::UVAT_group1 group1;
    group1.Hex = 0;
    gpu::UVAT_group2 group2;
    group2.Hex = 0;

    group1.pad = 1; // TODO: Is this undefined?

    group0.PosElements = static_cast<u32>(VertexComponentCount::Position::xyz);
    group0.PosFormat = static_cast<u32>(VertexBufferType::Generic::f32);
    group0.PosFrac = 0;

    group0.NormalElements = static_cast<u32>(VertexComponentCount::Normal::xyz);
    group0.NormalFormat = static_cast<u32>(VertexBufferType::Generic::f32);
    group0.NormalIndex3 = 0;

    group0.Color0Elements = static_cast<u32>(VertexComponentCount::Color::rgba);
    group0.Color0Comp = static_cast<u32>(VertexBufferType::Color::rgba8);

    group0.Color1Elements = static_cast<u32>(VertexComponentCount::Color::rgba);
    group0.Color1Comp = static_cast<u32>(VertexBufferType::Color::rgba8);

    const auto tex_st =
        static_cast<u32>(VertexComponentCount::TextureCoordinate::st);
    const auto tex_f32 = static_cast<u32>(VertexBufferType::Generic::f32);

    group0.Tex0CoordElements = tex_st;
    group0.Tex0CoordFormat = tex_f32;
    group0.Tex0Frac = 0;

    group1.Tex1CoordElements = tex_st;
    group1.Tex1CoordFormat = tex_f32;
    group1.Tex1Frac = 0;

    group1.Tex2CoordElements = tex_st;
    group1.Tex2CoordFormat = tex_f32;
    group1.Tex2Frac = 0;

    group1.Tex2CoordElements = tex_st;
    group1.Tex2CoordFormat = tex_f32;
    group1.Tex2Frac = 0;

    group1.Tex3CoordElements = tex_st;
    group1.Tex3CoordFormat = tex_f32;
    group1.Tex3Frac = 0;

    group1.Tex4CoordElements = tex_st;
    group1.Tex4CoordFormat = tex_f32;
    group2.Tex4Frac = 0;

    group2.Tex5CoordElements = tex_st;
    group2.Tex5CoordFormat = tex_f32;
    group2.Tex5Frac = 0;

    group2.Tex6CoordElements = tex_st;
    group2.Tex6CoordFormat = tex_f32;
    group2.Tex6Frac = 0;

    group2.Tex7CoordElements = tex_st;
    group2.Tex7CoordFormat = tex_f32;
    group2.Tex7Frac = 0;

    for (auto [attr, quant] : quants) {
      switch (attr) {
      case VertexAttribute::Position:
        group0.PosElements = static_cast<u32>(quant.comp.position);
        group0.PosFormat = static_cast<u32>(quant.type.generic);
        group0.PosFrac = quant.divisor;
        break;
      case VertexAttribute::Normal:
      case VertexAttribute::NormalBinormalTangent:
        group0.NormalFormat = static_cast<u32>(quant.type.generic);
        if (quant.comp.normal == VertexComponentCount::Normal::nbt3) {
          group0.NormalElements =
              static_cast<u32>(VertexComponentCount::Normal::nbt);
          group0.NormalIndex3 = true;
        } else {
          group0.NormalElements = static_cast<u32>(quant.comp.normal);
          group0.NormalIndex3 = false;
        }
        break;
      case VertexAttribute::Color0:
        group0.Color0Elements = static_cast<u32>(quant.comp.color);
        group0.Color0Comp = static_cast<u32>(quant.type.color);
        break;
      case VertexAttribute::Color1:
        group0.Color1Elements = static_cast<u32>(quant.comp.color);
        group0.Color1Comp = static_cast<u32>(quant.type.color);
        break;
      case VertexAttribute::TexCoord0:
        group0.Tex0CoordElements = static_cast<u32>(quant.comp.texcoord);
        group0.Tex0CoordFormat = static_cast<u32>(quant.type.generic);
        group0.Tex0Frac = quant.divisor;
        break;
      case VertexAttribute::TexCoord1:
        group1.Tex1CoordElements = static_cast<u32>(quant.comp.texcoord);
        group1.Tex1CoordFormat = static_cast<u32>(quant.type.generic);
        group1.Tex1Frac = quant.divisor;
        break;
      case VertexAttribute::TexCoord2:
        group1.Tex2CoordElements = static_cast<u32>(quant.comp.texcoord);
        group1.Tex2CoordFormat = static_cast<u32>(quant.type.generic);
        group1.Tex2Frac = quant.divisor;
        break;
      case VertexAttribute::TexCoord3:
        group1.Tex3CoordElements = static_cast<u32>(quant.comp.texcoord);
        group1.Tex3CoordFormat = static_cast<u32>(quant.type.generic);
        group1.Tex3Frac = quant.divisor;
        break;
      case VertexAttribute::TexCoord4:
        group1.Tex4CoordElements = static_cast<u32>(quant.comp.texcoord);
        group1.Tex4CoordFormat = static_cast<u32>(quant.type.generic);
        group2.Tex4Frac = quant.divisor;
        break;
      case VertexAttribute::TexCoord5:
        group2.Tex5CoordElements = static_cast<u32>(quant.comp.texcoord);
        group2.Tex5CoordFormat = static_cast<u32>(quant.type.generic);
        group2.Tex5Frac = quant.divisor;
        break;
      case VertexAttribute::TexCoord6:
        group2.Tex6CoordElements = static_cast<u32>(quant.comp.texcoord);
        group2.Tex6CoordFormat = static_cast<u32>(quant.type.generic);
        group2.Tex6Frac = quant.divisor;
        break;
      case VertexAttribute::TexCoord7:
        group2.Tex7CoordElements = static_cast<u32>(quant.comp.texcoord);
        group2.Tex7CoordFormat = static_cast<u32>(quant.type.generic);
        group2.Tex7Frac = quant.divisor;
        break;
      default:
        break;
      }
    }

    writeCp(0x70 + fmtIdx, group0.Hex);
    writeCp(0x80 + fmtIdx, group1.Hex);
    writeCp(0x90 + fmtIdx, group2.Hex);
  }

  enum {
    XF_MEMORY_POS_MTX = 0,
    XF_MEMORY_NRM_MTX = 0x400,
    XF_MEMORY_PTT_MTX = 0x500,
  };

  // GX matrix IDs are specified in units of rows, such that the same ID can
  // be used for both 3x4 position and 3x3 normal matrices.
  static u16 XFPosMtx(u32 row) { return XF_MEMORY_POS_MTX + row * 4; }
  // Texture and position matrices exist in the same space
  static u16 XFTexMtx(u32 row) { return XFPosMtx(row); }
  // Normal matrices exist in a parallel space
  static u16 XFNrmMtx(u32 row) { return XF_MEMORY_NRM_MTX + row * 3; }
  // Post-transform texture matrices exist in a sequestered address space,
  // however would seem to the user to exist in the same space as
  // texture/position matrices.
  static u16 XPTTMtx(u32 row) {
    return XF_MEMORY_PTT_MTX +
           (row - static_cast<u32>(librii::gx::PostTexMatrix::Matrix0)) * 4;
  }

  void loadPosMtxIndx(u16 from_idx, u32 to_idx) {
    writeXfIdxA(XFPosMtx(to_idx), 3 * 4, from_idx);
  }

  void loadNrmMtxIndx3x3(u16 from_idx, u32 to_idx) {
    writeXfIdxB(XFNrmMtx(to_idx), 3 * 3, from_idx);
  }

  void loadTexMtxIndx(u16 from_idx, u32 to_idx, librii::gx::TexGenType type) {
    if (to_idx >= static_cast<u32>(librii::gx::PostTexMatrix::Matrix0)) {
      assert(type == librii::gx::TexGenType::Matrix3x4);
      writeXfIdxC(XPTTMtx(to_idx), 3 * 4, from_idx);
    } else {
      writeXfIdxC(XFTexMtx(to_idx),
                  type == librii::gx::TexGenType::Matrix2x4 ? 2 * 4 : 3 * 4,
                  from_idx);
    }
  }

  void align() {
    while (mWriter.tell() % 32)
      mWriter.write<u8>(0);
  }
  void bpMask(u32 mask) { writeBp(BPAddress::BP_MASK, mask); }

private:
  enum class Command {
    BP = 0x61,
    XF = 0x10,
    CP = 0x08,
    XF_IDX_A = 0x20,
    XF_IDX_B = 0x28,
    XF_IDX_C = 0x30,
    XF_IDX_D = 0x38,
  };

  void writeBp(u8 reg, u32 val) {
    val &= static_cast<u32>((1 << 24) - 1); // 24-bit
    val |= (reg & 0xff) << 24;

    mWriter.writeUnaligned<u8>(static_cast<u8>(Command::BP));
    mWriter.writeUnaligned<u32>(val);
  }
  void writeBp(BPAddress reg, u32 val) { writeBp(static_cast<u8>(reg), val); }
  // 9 bytes
  void writeXf(u16 reg, u32 val) {
    mWriter.writeUnaligned<u8>(static_cast<u8>(Command::XF));
    mWriter.writeUnaligned<u16>(0);
    mWriter.writeUnaligned<u16>(reg);
    mWriter.writeUnaligned<u32>(val);
  }
  // 5 bytes
  void writeXfIdxA(u16 reg, u8 len, u16 index) {
    assert(index <= 0x0fff);
    assert((len - 1) <= 0xf);
    mWriter.writeUnaligned<u8>(static_cast<u8>(Command::XF_IDX_A));
    mWriter.writeUnaligned<u16>(index);
    mWriter.writeUnaligned<u16>((reg & 0x0fff) | (((len - 1) & 0xf) << 12));
  }
  // 5 bytes
  void writeXfIdxB(u16 reg, u8 len, u16 index) {
    assert(index <= 0x0fff);
    assert((len - 1) <= 0xf);
    mWriter.writeUnaligned<u8>(static_cast<u8>(Command::XF_IDX_B));
    mWriter.writeUnaligned<u16>(index);
    mWriter.writeUnaligned<u16>((reg & 0x0fff) | (((len - 1) & 0xf) << 12));
  }
  // 5 bytes
  void writeXfIdxC(u16 reg, u8 len, u16 index) {
    assert(index <= 0x0fff);
    assert((len - 1) <= 0xf);
    mWriter.writeUnaligned<u8>(static_cast<u8>(Command::XF_IDX_C));
    mWriter.writeUnaligned<u16>(index);
    mWriter.writeUnaligned<u16>((reg & 0x0fff) | (((len - 1) & 0xf) << 12));
  }
  // 5 bytes
  void writeXfIdxD(u16 reg, u8 len, u16 index) {
    assert(index <= 0x0fff);
    assert((len - 1) <= 0xf);
    mWriter.writeUnaligned<u8>(static_cast<u8>(Command::XF_IDX_D));
    mWriter.writeUnaligned<u16>(index);
    mWriter.writeUnaligned<u16>((reg & 0x0fff) | (((len - 1) & 0xf) << 12));
  }
  // 6 bytes
  void writeCp(u8 reg, u32 val) {
    mWriter.writeUnaligned<u8>(static_cast<u8>(Command::CP));
    mWriter.writeUnaligned<u8>(reg);
    mWriter.writeUnaligned<u32>(val);
  }

  oishii::Writer& mWriter;
};

} // namespace librii::gpu
