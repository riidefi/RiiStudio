#include "../Sections.hpp"
#include <map>
#include <string.h>

#include <plugins/gc/GPU/GPUAddressSpace.hpp>
#include <plugins/gc/GPU/GPUMaterial.hpp>

namespace riistudio::j3d {

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
                            libcube::gx::TextureFormat format) {
      assert(width <= 1024 && height <= 1024 &&
             "No dimension may exceed 1024x1024");
      std::array<u8, 8> lut{0x94, 0x95, 0x96, 0x97, 0xB4, 0xB5, 0xB6, 0xB7};

      constexpr u32 dimension_mask = (1 << 10) - 1;
      constexpr u32 format_mask = (1 << 4) - 1;

      mBuilder.writeBp(lut[mId], ((width - 1) & dimension_mask) |
                                     (((height - 1) & dimension_mask) << 10) |
                                     (static_cast<u32>(format) << 20));
    }
    void setLookupMode(libcube::gx::TextureWrapMode wrap_s,
                       libcube::gx::TextureWrapMode wrap_t,
                       libcube::gx::TextureFilter min_filt,
                       libcube::gx::TextureFilter mag_filt, f32 min_lod,
                       f32 max_lod, f32 lod_bias, bool clamp_bias,
                       bool edge_lod, libcube::gx::AnisotropyLevel aniso) {
      std::array<u8, 8> lut0{0x80, 0x81, 0x82, 0x83, 0xA0, 0xA1, 0xA2, 0xA3};
      std::array<u8, 8> lut1{0x84, 0x85, 0x86, 0x87, 0xA4, 0xA5, 0xA6, 0xA7};

      std::array<u8, 6> filter_lut{0, 4, 1, 5, 2, 6};

      libcube::gpu::TexMode0 mode0;
      mode0.hex = 0;
      libcube::gpu::TexMode1 mode1;
      mode1.hex = 0;

      mode0.wrap_s = static_cast<u32>(wrap_s);
      mode0.wrap_t = static_cast<u32>(wrap_t);
      mode0.mag_filter = mag_filt == libcube::gx::TextureFilter::linear;
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
  void loadTLUT() {}

  void setTevOrder() {}
  void setTexCoordScale2() {}

  void setTevColor() {}
  void setTevKColor() {}
  void setTevColorCalc() {}
  void setTevAlphaCalcAndSwap() {}
  void setTevIndirect() {}
  void setTevKonstantSelAndSwapModeTable() {}
  void setIndTexMtx() {}
  void setIndTexCoordScale() {}

  void setFog() {}
  void setFogRangeAdj() {}
  void setAlphaCompare() {}
  void setBlendMode() {}
  void setZMode() {}

  void setChanColor() {}
  void setAmbColor() {}

private:
  enum class Command { BP = 0x61 };

  void writeBp(u8 reg, u32 val) {
    val &= static_cast<u32>((1 << 24) - 1); // 24-bit
    val |= (reg & 0xff) << 24;

    mWriter.writeUnaligned<u8>(static_cast<u8>(Command::BP));
    mWriter.writeUnaligned<u32>(val);
  }
  void writeBp(BPAddress reg, u32 val) { writeBp(static_cast<u8>(reg), val); }

  oishii::Writer& mWriter;
};

template <typename T> static void writeAt(T& stream, u32 pos, s32 val) {
  auto back = stream.tell();
  stream.seekSet(pos);
  stream.write<s32>(val);
  stream.seekSet(back);
}
template <typename T> static void writeAtS16(T& stream, u32 pos, s16 val) {
  auto back = stream.tell();
  stream.seekSet(pos);
  stream.write<s16>(val);
  stream.seekSet(back);
}

// TODO
struct MDL3Node final : public oishii::Node {
  MDL3Node(const ModelAccessor model, const CollectionAccessor col)
      : mModel(model), mCol(col) {
    mId = "MDL3";
    mLinkingRestriction.alignment = 32;
  }

  Result write(oishii::Writer& writer) const noexcept override {
    const auto& mats = mModel.getMaterials();
    const auto start = writer.tell();

    writer.write<u32, oishii::EndianSelect::Big>('MDL3');
    writer.writeLink<s32>({*this}, {*this, oishii::Hook::EndOfChildren});

    writer.write<u16>((u32)mats.size());
    writer.write<u16>(-1);

    const auto ofsDls = writer.tell();
    writer.write<u32>(32 + mats.size() * 6 * sizeof(u32));
    const auto ofsDlHdrs = writer.tell();
    writer.write<u32>(32);

    const auto ofsSrMtxIdx = writer.tell();
    writer.write<u32>(0);
    const auto ofsLut = writer.tell();
    writer.write<u32>(0);
    const auto ofsNameTable = writer.tell();
    writer.write<u32>(0);

    const auto dlHandlesOfs = writer.tell();
    // dl data (offset, size)
    for (int i = 0; i < mats.size(); ++i) {
      writer.write<u32>(0);
      writer.write<u32>(0);
    }

    const auto dlHeadersOfs = writer.tell();
    // Simplify by writing the headers first
    // (6 offsets)
    for (int i = 0; i < mats.size() * 6; ++i)
      writer.write<u16>(0);

    const auto dlDataOfs = writer.tell();
    for (int i = 0; i < mats.size(); ++i) {
      const auto& mat = mModel.getMaterial(i).get();
      const auto dl_start = writer.tell();
      writeAt(writer, dlHandlesOfs + 8 * i + 0,
              writer.tell() - dlHandlesOfs * 8 + i);
      DLBuilder builder(writer);

      // write texture dl
      writeAtS16(writer, dlHeadersOfs + i * 24 + 12,
                 writer.tell() - (dlHeadersOfs + i * 24 + 12));
      {
        for (int i = 0; i < mat.samplers.size(); ++i) {
          const auto& sampler =
              *reinterpret_cast<const Material::J3DSamplerData*>(
                  mat.samplers[i].get());
          const auto& image = mModel.get().mTexCache[sampler.btiId];

          auto tex_delegate = builder.setTexture(i);

          tex_delegate.setImagePointer(sampler.btiId, true);
          tex_delegate.setImageAttributes(image.mWidth, image.mHeight,
                                          image.mFormat);
          tex_delegate.setLookupMode(sampler.mWrapU, sampler.mWrapV,
                                     sampler.mMinFilter, sampler.mMagFilter,
                                     image.mMinLod, image.mMaxLod,
                                     sampler.mLodBias, sampler.bBiasClamp,
                                     sampler.bEdgeLod, sampler.mMaxAniso);

          if (image.mFormat == libcube::gx::TextureFormat::C4 ||
              image.mFormat == libcube::gx::TextureFormat::C8 ||
              image.mFormat == libcube::gx::TextureFormat::C14X2) {
            // TODO: implement
            builder.loadTLUT();
            tex_delegate.setTLUT();
          }
        }

        const auto& stages = mat.getMaterialData().shader.mStages;
        for (int i = 0; i < stages.size(); ++i) {
          const auto& stage = stages[i];

          if (i % 2 == 0)
            builder.setTevOrder();

          builder.setTexCoordScale2();
        }
      }
      // write tev dl
      writeAtS16(writer, dlHeadersOfs + i * 24 + 16,
                 writer.tell() - (dlHeadersOfs + i * 24 + 16));
      {
        // Don't set prev
        for (int i = 0; i < 3; ++i)
          ; // builder.setTevColor(reg, i + 1);
        for (int i = 0; i < 4; ++i)
          ; // builder.setTevKColor(reg, i);
        for (int i = 0; i < mat.shader.mStages.size(); ++i) {
          builder.setTevColorCalc();
          builder.setTevAlphaCalcAndSwap();
          builder.setTevIndirect();
        }
        for (int i = 0; i < 8; ++i)
          builder.setTevKonstantSelAndSwapModeTable();
        for (int i = 0; i < mat.info.nIndStage; ++i) {
          builder.setIndTexMtx();
        }
        for (int i = 0; i < mat.info.nIndStage; ++i) {
          if (i % 2 == 0)
            builder.setIndTexCoordScale();
        }
        for (int i = 0; i < mat.info.nIndStage; ++i) {
          // mask 0x03FFFF
          // SU_SSIZE, SU_TSIZE
        }

        for (int i = 0; i < 4 - mat.info.nIndStage; ++i) {
          // mask 0x03FFFF, 0x2e, 0x2f
          // Not valid, though will be corrected later in DL
        }
        // IREF
        // IND_IMASK
      }
      // write pe dl
      writeAtS16(writer, dlHeadersOfs + i * 24 + 20,
                 writer.tell() - (dlHeadersOfs + i * 24 + 20));
      {
        builder.setFog();
        builder.setFogRangeAdj();
        builder.setAlphaCompare();
        builder.setBlendMode();
        builder.setZMode();
        // ZCOMPARE / PECNTRL
        // GENMODE
      }
      // write tg dl
      writeAtS16(writer, dlHeadersOfs + i * 24 + 8,
                 writer.tell() - (dlHeadersOfs + i * 24 + 8));
      {
        //
      }
      // write chan color dl
      writeAtS16(writer, dlHeadersOfs + i * 24 + 0,
                 writer.tell() - (dlHeadersOfs + i * 24 + 0));
      {
        builder.setChanColor();
        builder.setAmbColor();
      }
      // write chan control dl
      writeAtS16(writer, dlHeadersOfs + i * 24 + 4,
                 writer.tell() - (dlHeadersOfs + i * 24 + 4));
      {
        //
      }

      // align 32 bytes
      while (writer.tell() % 32)
        writer.write<u8>(0);

      // write size
      writeAt(writer, dlHandlesOfs + 8 * i + 4, writer.tell() - dl_start);
    }
    return eResult::Success;
  }

private:
  const ModelAccessor mModel;
  const CollectionAccessor mCol;
};

std::unique_ptr<oishii::Node> makeMDL3Node(BMDExportContext& ctx) {
  return std::make_unique<MDL3Node>(ctx.mdl, ctx.col);
}

} // namespace riistudio::j3d
