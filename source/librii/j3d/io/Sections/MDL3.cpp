#include "../Sections.hpp"
#include <librii/gpu/DLBuilder.hpp>

IMPORT_STD;

namespace librii::j3d {

using DLBuilder = librii::gpu::DLBuilder;

template <typename T> static void writeAt(T& stream, u32 pos, s32 val) {
  auto back = stream.tell();
  stream.seekSet(pos);
  stream.template write<s32>(val);
  stream.seekSet(back);
}
template <typename T> static void writeAtS16(T& stream, u32 pos, s16 val) {
  auto back = stream.tell();
  stream.seekSet(pos);
  stream.template write<s16>(val);
  stream.seekSet(back);
}

// TODO
struct MDL3Node final : public oishii::Node {
  MDL3Node(const BMDExportContext& model) : mModel(model) {
    mId = "MDL3";
    mLinkingRestriction.alignment = 32;
  }

  Result<void> write(oishii::Writer& writer) const noexcept override {
    const auto& mats = mModel.mdl.materials;
    [[maybe_unused]] const auto start = writer.tell();

    writer.write<u32, oishii::EndianSelect::Big>('MDL3');
    writer.writeLink<s32>({*this}, {*this, oishii::Hook::EndOfChildren});

    writer.write<u16>((u32)mats.size());
    writer.write<u16>(-1);

    [[maybe_unused]] const auto ofsDls = writer.tell();
    writer.write<u32>(32 + mats.size() * 6 * sizeof(u32));
    [[maybe_unused]] const auto ofsDlHdrs = writer.tell();
    writer.write<u32>(32);

    [[maybe_unused]] const auto ofsSrMtxIdx = writer.tell();
    writer.write<u32>(0);
    [[maybe_unused]] const auto ofsLut = writer.tell();
    writer.write<u32>(0);
    [[maybe_unused]] const auto ofsNameTable = writer.tell();
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

    // const auto dlDataOfs = writer.tell();
    for (int i = 0; i < mats.size(); ++i) {
      const auto& mat = mModel.mdl.materials[i];
      const auto dl_start = writer.tell();
      writeAt(writer, dlHandlesOfs + 8 * i + 0,
              writer.tell() - dlHandlesOfs * 8 + i);
      DLBuilder builder(writer);

      // write texture dl
      writeAtS16(writer, dlHeadersOfs + i * 24 + 12,
                 writer.tell() - (dlHeadersOfs + i * 24 + 12));
      {
        for (int i = 0; i < mat.samplers.size(); ++i) {
          const auto& sampler = mat.samplers[i];
          const auto& image = mModel.mTexCache[sampler.btiId];

          auto tex_delegate = builder.setTexture(i);

          tex_delegate.setImagePointer(sampler.btiId, true);
          tex_delegate.setImageAttributes(image.mWidth, image.mHeight,
                                          image.mFormat);
          tex_delegate.setLookupMode(sampler.mWrapU, sampler.mWrapV,
                                     sampler.mMinFilter, sampler.mMagFilter,
                                     image.mMinLod, image.mMaxLod,
                                     sampler.mLodBias, sampler.bBiasClamp,
                                     sampler.bEdgeLod, sampler.mMaxAniso);

          if (image.mFormat == librii::gx::TextureFormat::C4 ||
              image.mFormat == librii::gx::TextureFormat::C8 ||
              image.mFormat == librii::gx::TextureFormat::C14X2) {
            // TODO: implement
            builder.loadTLUT();
            tex_delegate.setTLUT();
          }
        }

        const auto& stages = mat.mStages;
        for (int i = 0; i < stages.size(); ++i) {
          const auto& stage = stages[i];
          (void)stage;

          if (i % 2 == 0)
            ; // builder.setTevOrder();

          builder.setTexCoordScale2();
        }
      }
      // write tev dl
      writeAtS16(writer, dlHeadersOfs + i * 24 + 16,
                 writer.tell() - (dlHeadersOfs + i * 24 + 16));
      {
        // Don't set prev
        for (int i = 0; i < 3; ++i)
          builder.setTevColor(i + 1, mat.tevColors[i]);
        for (int i = 0; i < 4; ++i)
          builder.setTevKColor(i, mat.tevKonstColors[i]);
        for (int i = 0; i < mat.mStages.size(); ++i) {
          builder.setTevColorCalc(i, mat.mStages[i].colorStage);
          builder.setTevAlphaCalcAndSwap(i, mat.mStages[i].alphaStage,
                                         mat.mStages[i].rasSwap,
                                         mat.mStages[i].texMapSwap);
          builder.setTevIndirect(i, mat.mStages[i].indirectStage);
        }
        for (int i = 0; i < 8; ++i)
          builder.setTevKonstantSelAndSwapModeTable();
        for (int i = 0; i < mat.indirectStages.size(); ++i) {
          builder.setIndTexMtx(i, mat.mIndMatrices[i]);
        }
        for (int i = 0; i < mat.indirectStages.size(); ++i) {
          if (i % 2 == 0)
            builder.setIndTexCoordScale(i, mat.indirectStages[i].scale,
                                        mat.indirectStages[i + 1].scale);
        }
        // TODO: PAD?
        for (int i = 0; i < mat.indirectStages.size(); ++i) {
          // mask 0x03FFFF
          // SU_SSIZE, SU_TSIZE
        }

        for (int i = 0; i < 4 - mat.indirectStages.size(); ++i) {
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
        builder.setAlphaCompare(mat.alphaCompare);
        builder.setBlendMode(mat.blendMode);
        builder.setZMode(mat.zMode);
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
    return {};
  }

private:
  BMDExportContext mModel;
};

std::unique_ptr<oishii::Node> makeMDL3Node(BMDExportContext& ctx) {
  return std::make_unique<MDL3Node>(ctx);
}

} // namespace librii::j3d
