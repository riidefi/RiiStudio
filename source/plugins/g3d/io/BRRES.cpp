#include <core/common.h>
#include <core/kpi/Node.hpp>

#include <oishii/reader/binary_reader.hxx>
#include <oishii/writer/binary_writer.hxx>

#include <plugins/g3d/collection.hpp>

#include <plugins/g3d/util/Dictionary.hpp>
#include <plugins/g3d/util/NameTable.hpp>

#include <string>

#include <plugins/gc/GPU/DLPixShader.hpp>
#include <plugins/gc/GPU/GPUMaterial.hpp>

#include <plugins/gc/GX/VertexTypes.hpp>

#include <core/util/glm_io.hpp>

inline void operator<<(libcube::gx::Color& out, oishii::BinaryReader& reader) {
  out = libcube::gx::readColorComponents(
      reader, libcube::gx::VertexBufferType::Color::rgba8);
}

namespace riistudio::g3d {

template <typename T, bool HasMinimum, bool HasDivisor,
          libcube::gx::VertexBufferKind kind>
void readGenericBuffer(GenericBuffer<T, HasMinimum, HasDivisor, kind>& out,
                       oishii::BinaryReader& reader) {
  const auto start = reader.tell();
  out.mEntries.clear();

  reader.read<u32>(); // size
  reader.read<u32>(); // mdl0 offset
  const auto startOfs = reader.read<s32>();
  out.mName = readName(reader, start);
  out.mId = reader.read<u32>();
  out.mQuantize.mComp = libcube::gx::VertexComponentCount(
      static_cast<libcube::gx::VertexComponentCount::Normal>(
          reader.read<u32>()));
  out.mQuantize.mType = libcube::gx::VertexBufferType(
      static_cast<libcube::gx::VertexBufferType::Color>(reader.read<u32>()));
  if (HasDivisor) {
    out.mQuantize.divisor = reader.read<u8>();
    out.mQuantize.stride = reader.read<u8>();
  } else {
    out.mQuantize.divisor = 0;
    out.mQuantize.stride = reader.read<u8>();
    reader.read<u8>();
  }
  out.mEntries.resize(reader.read<u16>());
  T minEnt, maxEnt;
  if (HasMinimum) {
    minEnt << reader;
    maxEnt << reader;
  }
  const auto nComponents =
      libcube::gx::computeComponentCount(kind, out.mQuantize.mComp);
  assert((kind != libcube::gx::VertexBufferKind::normal) ||
         (!((int)out.mQuantize.divisor != 6 &&
            out.mQuantize.mType.generic ==
                libcube::gx::VertexBufferType::Generic::s8) &&
          !((int)out.mQuantize.divisor != 14 &&
            out.mQuantize.mType.generic ==
                libcube::gx::VertexBufferType::Generic::s16)));
  assert(kind != libcube::gx::VertexBufferKind::normal ||
         (out.mQuantize.mType.generic !=
              libcube::gx::VertexBufferType::Generic::u8 &&
          out.mQuantize.mType.generic !=
              libcube::gx::VertexBufferType::Generic::u16));

  reader.seekSet(start + startOfs);
  // TODO: Recompute bounds
  for (auto& entry : out.mEntries) {
    entry = libcube::gx::readComponents<T>(reader, out.mQuantize.mType,
                                           nComponents, out.mQuantize.divisor);
  }
}

enum class RenderCommand {
  NoOp,
  Return,

  NodeDescendence,
  NodeMixing,

  Draw,

  EnvelopeMatrix,
  MatrixCopy

};

static void readModel(Model& mdl, oishii::BinaryReader& reader,
                      kpi::IOTransaction& transaction,
                      const std::string& transaction_path) {
  const auto start = reader.tell();

  reader.expectMagic<'MDL0', true>();

  const u32 fileSize = reader.read<u32>();
  (void)fileSize;
  const u32 revision = reader.read<u32>();
  if (revision != 11) {
    transaction.callback(kpi::IOMessageClass::Error, transaction_path,
                         "MDL0 is version " + std::to_string(revision) +
                             ". Only MDL0 version 11 is supported.");
    return;
  }

  reader.read<s32>(); // ofsBRRES

  union {
    struct {
      s32 ofsRenderTree;
      s32 ofsBones;
      struct {
        s32 position;
        s32 normal;
        s32 color;
        s32 uv;
        s32 furVec;
        s32 furPos;
      } ofsBuffers;
      s32 ofsMaterials;
      s32 ofsShaders;
      s32 ofsMeshes;
      s32 ofsTextureLinks;
      s32 ofsPaletteLinks;
      s32 ofsUserData;
    } secOfs;
    std::array<s32, 14> secOfsArr;
  };
  for (auto& ofs : secOfsArr)
    ofs = reader.read<s32>();

  mdl.setName(readName(reader, start));

  const auto infoPos = reader.tell();
  reader.skip(8); // Ignore size, ofsMode
  mdl.mScalingRule = static_cast<ScalingRule>(reader.read<u32>());
  mdl.mTexMtxMode = static_cast<TextureMatrixMode>(reader.read<u32>());

  reader.readX<u32, 2>(); // number of vertices, number of triangles
  mdl.sourceLocation = readName(reader, infoPos);
  reader.read<u32>(); // number of view matrices

  // const auto [bMtxArray, bTexMtxArray, bBoundVolume] =
  reader.readX<u8, 3>();
  mdl.mEvpMtxMode = static_cast<EnvelopeMatrixMode>(reader.read<u8>());

  // const s32 ofsBoneTable =
  reader.read<s32>();

  mdl.aabb.min << reader;
  mdl.aabb.max << reader;

  auto readDict = [&](u32 xofs, auto handler) {
    if (xofs) {
      reader.seekSet(start + xofs);
      Dictionary _dict(reader);
      for (std::size_t i = 1; i < _dict.mNodes.size(); ++i) {
        const auto& dnode = _dict.mNodes[i];
        assert(dnode.mDataDestination);
        reader.seekSet(dnode.mDataDestination);
        handler(dnode);
      }
    }
  };

  readDict(secOfs.ofsBones, [&](const DictionaryNode& dnode) {
    auto& bone = mdl.getBones().add();
    reader.skip(8); // skip size and mdl offset
    bone.setName(readName(reader, dnode.mDataDestination));
    bone.mId = reader.read<u32>();
    bone.matrixId = reader.read<u32>();
    bone.flag = reader.read<u32>();
    bone.billboardType = reader.read<u32>();
    reader.read<u32>(); // refId
    bone.mScaling << reader;
    bone.mRotation << reader;
    bone.mTranslation << reader;

    bone.mVolume.min << reader;
    bone.mVolume.max << reader;

    auto readHierarchyElement = [&]() {
      const auto ofs = reader.read<s32>();
      if (ofs == 0)
        return -1;
      // skip to id
      oishii::Jump(reader, dnode.mDataDestination + ofs + 12);
      return static_cast<s32>(reader.read<u32>());
    };
    bone.mParent = readHierarchyElement();
    reader.skip(12); // Skip sibling and child links -- we recompute it all
    reader.skip(2 * ((3 * 4) * sizeof(f32))); // skip matrices
  });
  // Compute children
  for (int i = 0; i < mdl.getBones().size(); ++i) {
    if (const auto parent_id = mdl.getBones()[i].mParent; parent_id >= 0) {
      mdl.getBones()[parent_id].mChildren.push_back(i);
    }
  }
  readDict(secOfs.ofsBuffers.position, [&](const DictionaryNode& dnode) {
    readGenericBuffer(mdl.getBuf_Pos().add(), reader);
  });
  readDict(secOfs.ofsBuffers.normal, [&](const DictionaryNode& dnode) {
    readGenericBuffer(mdl.getBuf_Nrm().add(), reader);
  });
  readDict(secOfs.ofsBuffers.color, [&](const DictionaryNode& dnode) {
    readGenericBuffer(mdl.getBuf_Clr().add(), reader);
  });
  readDict(secOfs.ofsBuffers.uv, [&](const DictionaryNode& dnode) {
    readGenericBuffer(mdl.getBuf_Uv().add(), reader);
  });
  // TODO: Fur

  readDict(secOfs.ofsMaterials, [&](const DictionaryNode& dnode) {
    auto& mat = mdl.getMaterials().add();
    const auto start = reader.tell();

    reader.read<u32>(); // size
    reader.read<s32>(); // mdl offset
    mat.name = readName(reader, start);
    mat.id = reader.read<u32>();
    mat.flag = reader.read<u32>();

    // Gen info
    mat.info.nTexGen = reader.read<u8>();
    mat.info.nColorChan = reader.read<u8>();
    mat.info.nTevStage = reader.read<u8>();
    mat.info.nIndStage = reader.read<u8>();
    mat.cullMode = static_cast<libcube::gx::CullMode>(reader.read<u32>());

    // Misc
    mat.earlyZComparison = reader.read<u8>();
    mat.lightSetIndex = reader.read<s8>();
    mat.fogIndex = reader.read<s8>();
    reader.read<u8>();
    const auto indSt = reader.tell();
    for (u8 i = 0; i < mat.info.nIndStage; ++i) {
      G3dIndConfig cfg;
      cfg.method = static_cast<G3dIndMethod>(reader.read<u8>());
      cfg.normalMapLightRef = reader.peekAt<s8>(4);
      mat.indConfig.push_back(cfg);
    }
    reader.seekSet(indSt + 4 + 4);

    const auto ofsTev = reader.read<s32>();
    const auto nTex = reader.read<u32>();
    assert(nTex <= 8);
    const auto [ofsSamplers, ofsFur, ofsUserData, ofsDisplayLists] =
        reader.readX<s32, 4>();
    if (ofsFur || ofsUserData)
      printf(
          "Warning: Material %s uses Fur or UserData which is unsupported!\n",
          mat.name.c_str());

    // Texture and palette objects are set on runtime.
    reader.skip((4 + 8 * 12) + (4 + 8 * 32));

    // Texture transformations
    reader.read<u32>(); // skip flag, TODO: verify
    const u32 mtxMode = reader.read<u32>();
    const std::array<libcube::GCMaterialData::CommonTransformModel, 3> cvtMtx{
        libcube::GCMaterialData::CommonTransformModel::Maya,
        libcube::GCMaterialData::CommonTransformModel::XSI,
        libcube::GCMaterialData::CommonTransformModel::Max};
    const auto xfModel = cvtMtx[mtxMode];

    for (u8 i = 0; i < nTex; ++i) {
      auto mtx = std::make_unique<libcube::GCMaterialData::TexMatrix>();
      mtx->scale << reader;
      mtx->rotate = reader.read<f32>();
      mtx->translate << reader;
      mat.texMatrices.push_back(std::move(mtx));
    }
    reader.seekSet(start + 0x250);
    for (u8 i = 0; i < nTex; ++i) {
      auto& mtx = mat.texMatrices[i];

      mtx->camIdx = reader.read<s8>();
      mtx->lightIdx = reader.read<s8>();
      const u8 mapMode = reader.read<u8>();

      mtx->transformModel = xfModel;
      mtx->option = libcube::GCMaterialData::CommonMappingOption::NoSelection;
      // Projection needs to be copied from texgen

      switch (mapMode) {
      default:
      case 0:
        mtx->method = libcube::GCMaterialData::CommonMappingMethod::Standard;
        break;
      case 1:
        mtx->method =
            libcube::GCMaterialData::CommonMappingMethod::EnvironmentMapping;
        break;
      case 2:
        mtx->method =
            libcube::GCMaterialData::CommonMappingMethod::ViewProjectionMapping;
        break;
      case 3:
        mtx->method = libcube::GCMaterialData::CommonMappingMethod::
            EnvironmentLightMapping;
        break;
      case 4:
        mtx->method = libcube::GCMaterialData::CommonMappingMethod::
            EnvironmentSpecularMapping;
        break;
      // EGG
      case 5:
        mtx->method = libcube::GCMaterialData::CommonMappingMethod::
            ManualProjectionMapping;
        break;
      }

      reader.read<u8>(); // effect mtx flag
      for (auto& f : mtx->effectMatrix)
        f = reader.read<f32>();
    }
    // reader.seek((8 - nTex)* (4 + (4 * 4 * sizeof(f32))));
    reader.seekSet(start + 0x3f0);
    mat.chanData.nElements = 0;
    for (u8 i = 0; i < mat.info.nColorChan; ++i) {
      // skip runtime flag
      reader.read<u32>();
      libcube::gx::Color matClr, ambClr;
      matClr.r = reader.read<u8>();
      matClr.g = reader.read<u8>();
      matClr.b = reader.read<u8>();
      matClr.a = reader.read<u8>();

      ambClr.r = reader.read<u8>();
      ambClr.g = reader.read<u8>();
      ambClr.b = reader.read<u8>();
      ambClr.a = reader.read<u8>();

      mat.chanData.push_back({matClr, ambClr});
      libcube::gpu::LitChannel tmp;
      // Color
      tmp.hex = reader.read<u32>();
      mat.colorChanControls.push_back(tmp);
      // Alpha
      tmp.hex = reader.read<u32>();
      mat.colorChanControls.push_back(tmp);
    }

    // TEV
    reader.seekSet(start + ofsTev + 32);
    {
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
      libcube::gpu::QDisplayListShaderHandler shaderHandler(mat.shader,
                                                            mat.info.nTevStage);
      libcube::gpu::RunDisplayList(reader, shaderHandler,
                                   shaderDlSizes[mat.info.nTevStage]);
    }

    // Samplers
    reader.seekSet(start + ofsSamplers);
    for (u8 i = 0; i < mat.info.nTexGen; ++i) {
      auto sampler = std::make_unique<libcube::GCMaterialData::SamplerData>();

      const auto sampStart = reader.tell();
      sampler->mTexture = readName(reader, sampStart);
      sampler->mPalette = readName(reader, sampStart);
      reader.skip(8);     // skip runtime pointers
      reader.read<u32>(); // skip tex id for now
      reader.read<u32>(); // skip tlut id for now
      sampler->mWrapU =
          static_cast<libcube::gx::TextureWrapMode>(reader.read<u32>());
      sampler->mWrapV =
          static_cast<libcube::gx::TextureWrapMode>(reader.read<u32>());
      sampler->mMinFilter =
          static_cast<libcube::gx::TextureFilter>(reader.read<u32>());
      sampler->mMagFilter =
          static_cast<libcube::gx::TextureFilter>(reader.read<u32>());
      sampler->mLodBias = reader.read<f32>();
      sampler->mMaxAniso =
          static_cast<libcube::gx::AnisotropyLevel>(reader.read<u32>());
      sampler->bBiasClamp = reader.read<u8>();
      sampler->bEdgeLod = reader.read<u8>();
      reader.skip(2);
      mat.samplers.push_back(std::move(sampler));
    }

    // Display Lists
    reader.seekSet(start + ofsDisplayLists);
    {
      libcube::gpu::QDisplayListMaterialHandler matHandler(mat);

      // Pixel data
      libcube::gpu::RunDisplayList(reader, matHandler, 32);
      mat.alphaCompare = matHandler.mGpuMat.mPixel.mAlphaCompare;
      mat.zMode = matHandler.mGpuMat.mPixel.mZMode;
      mat.blendMode = matHandler.mGpuMat.mPixel.mBlendMode;
      // TODO: Dst alpha

      // Uniform data
      libcube::gpu::RunDisplayList(reader, matHandler, 128);
      mat.tevColors[0] = {0xff, 0xff, 0xff, 0xff};
      mat.tevColors.nElements = 4;
      for (int i = 0; i < 3; ++i) {
        mat.tevColors[i + 1] = matHandler.mGpuMat.mShaderColor.Registers[i + 1];
      }
      mat.tevKonstColors.nElements = 4;
      for (int i = 0; i < 4; ++i) {
        mat.tevKonstColors[i] = matHandler.mGpuMat.mShaderColor.Konstants[i];
      }

      // Indirect data
      libcube::gpu::RunDisplayList(reader, matHandler, 64);
      for (u8 i = 0; i < mat.info.nIndStage; ++i) {
        const auto& curScale =
            matHandler.mGpuMat.mIndirect.mIndTexScales[i > 1 ? i - 2 : i];
        mat.mIndScales.push_back(
            {static_cast<libcube::gx::IndirectTextureScalePair::Selection>(
                 curScale.ss0),
             static_cast<libcube::gx::IndirectTextureScalePair::Selection>(
                 curScale.ss1)});

        mat.mIndMatrices.push_back(
            matHandler.mGpuMat.mIndirect.mIndMatrices[i]);
      }

      const std::array<u32, 9> texGenDlSizes{
          0,        // 0
          32,       // 1
          64,  64,  // 2, 3
          96,  96,  // 4, 5
          128, 128, // 6, 7
          160       // 8
      };
      libcube::gpu::RunDisplayList(reader, matHandler,
                                   texGenDlSizes[mat.info.nTexGen]);
      for (u8 i = 0; i < mat.info.nTexGen; ++i) {
        mat.texGens.push_back(matHandler.mGpuMat.mTexture[i]);
        mat.texMatrices[i]->projection = mat.texGens[i].func;
      }
    }
  });
  readDict(secOfs.ofsMeshes, [&](const DictionaryNode& dnode) {
    auto& poly = mdl.getMeshes().add();
    const auto start = reader.tell();

    reader.read<u32>(); // size
    reader.read<s32>(); // mdl offset

    poly.mCurrentMatrix = reader.read<s32>();
    reader.skip(12); // cache

    struct DlHandle {
      std::size_t tag_start;
      std::size_t buf_size;
      std::size_t cmd_size;
      s32 ofs_buf;

      void seekTo(oishii::BinaryReader& reader) {
        reader.seekSet(tag_start + ofs_buf);
      }
      DlHandle(oishii::BinaryReader& reader) : tag_start(reader.tell()) {
        buf_size = reader.read<u32>();
        cmd_size = reader.read<u32>();
        ofs_buf = reader.read<s32>();
      }
    };

    DlHandle primitiveSetup(reader);
    DlHandle primitiveData(reader);

    poly.mVertexDescriptor.mBitfield = reader.read<u32>();
    const u32 flag = reader.read<u32>();
    poly.currentMatrixEmbedded = flag & 1;
    assert(!poly.currentMatrixEmbedded);
    poly.visible = !(flag & 2);

    poly.mName = readName(reader, start);
    poly.mId = reader.read<u32>();
    // TODO: Verify / cache
    reader.read<u32>(); // nVert
    reader.read<u32>(); // nPoly

    auto readBufHandle = [&](std::string& out, auto ifExist) {
      const auto hid = reader.read<s16>();
      if (hid < 0)
        out = "";
      else
        out = ifExist(hid);
    };

    readBufHandle(poly.mPositionBuffer,
                  [&](s16 hid) { return mdl.getBuf_Pos()[hid].getName(); });
    readBufHandle(poly.mNormalBuffer,
                  [&](s16 hid) { return mdl.getBuf_Nrm()[hid].getName(); });
    for (int i = 0; i < 2; ++i) {
      readBufHandle(poly.mColorBuffer[i],
                    [&](s16 hid) { return mdl.getBuf_Clr()[hid].getName(); });
    }
    for (int i = 0; i < 8; ++i) {
      readBufHandle(poly.mTexCoordBuffer[i],
                    [&](s16 hid) { return mdl.getBuf_Uv()[hid].getName(); });
    }
    reader.read<s32>(); // fur
    reader.read<s32>(); // matrix usage

    primitiveSetup.seekTo(reader);
    libcube::gpu::QDisplayListVertexSetupHandler vcdHandler;
    libcube::gpu::RunDisplayList(reader, vcdHandler, primitiveSetup.buf_size);

    for (u32 i = 0; i < (u32)libcube::gx::VertexAttribute::Max; ++i) {
      if (poly.mVertexDescriptor.mBitfield & (1 << i)) {
        const auto stat = vcdHandler.mGpuMesh.VCD.GetVertexArrayStatus(
            i - (u32)libcube::gx::VertexAttribute::Position);
        const auto att = static_cast<libcube::gx::VertexAttributeType>(stat);
        assert(att != libcube::gx::VertexAttributeType::None);
        poly.mVertexDescriptor.mAttributes[(libcube::gx::VertexAttribute)i] =
            att;
      }
    }
    struct QDisplayListMeshHandler final
        : public libcube::gpu::QDisplayListHandler {
      void onCommandDraw(oishii::BinaryReader& reader,
                         libcube::gx::PrimitiveType type, u16 nverts) override {
        if (mPoly.mMatrixPrimitives.empty())
          mPoly.mMatrixPrimitives.push_back(MatrixPrimitive{});
        auto& prim = mPoly.mMatrixPrimitives.back().mPrimitives.emplace_back(
            libcube::IndexedPrimitive{});
        prim.mType = type;
        prim.mVertices.resize(nverts);
        for (auto& vert : prim.mVertices) {
          for (u32 i = 0;
               i < static_cast<u32>(libcube::gx::VertexAttribute::Max); ++i) {
            if (mPoly.mVertexDescriptor.mBitfield & (1 << i)) {
              const auto attr = static_cast<libcube::gx::VertexAttribute>(i);
              switch (mPoly.mVertexDescriptor.mAttributes[attr]) {
              case libcube::gx::VertexAttributeType::Direct:
                throw "TODO";
                break;
              case libcube::gx::VertexAttributeType::None:
                break;
              case libcube::gx::VertexAttributeType::Byte:
                vert[attr] = reader.readUnaligned<u8>();
                break;
              case libcube::gx::VertexAttributeType::Short:
                vert[attr] = reader.readUnaligned<u16>();
                break;
              }
            }
          }
        }
      }
      QDisplayListMeshHandler(Polygon& poly) : mPoly(poly) {}
      Polygon& mPoly;
    } meshHandler(poly);
    primitiveData.seekTo(reader);
    libcube::gpu::RunDisplayList(reader, meshHandler, primitiveData.buf_size);
  });

  readDict(secOfs.ofsRenderTree, [&](const DictionaryNode& dnode) {
    enum class Tree { DrawOpa, DrawXlu, Node, Other } tree = Tree::Other;
    std::array<std::pair<std::string_view, Tree>, 3> type_lut{
        std::pair<std::string_view, Tree>{"DrawOpa", Tree::DrawOpa},
        std::pair<std::string_view, Tree>{"DrawXlu", Tree::DrawXlu},
        std::pair<std::string_view, Tree>{"NodeTree", Tree::Node}};

    if (const auto it = std::find_if(
            type_lut.begin(), type_lut.end(),
            [&](const auto& tuple) { return tuple.first == dnode.mName; });
        it != type_lut.end()) {
      tree = it->second;
    }
    int base_matrix_id = -1;

    // printf("digraph {\n");
    while (reader.tell() < reader.endpos()) {
      const auto cmd = static_cast<RenderCommand>(reader.read<u8>());
      switch (cmd) {
      case RenderCommand::NoOp:
        break;
      case RenderCommand::Return:
        return;
      case RenderCommand::Draw: {
        Bone::Display disp;
        disp.matId = reader.readUnaligned<u16>();
        disp.polyId = reader.readUnaligned<u16>();
        const auto boneIdx = reader.readUnaligned<u16>();
        disp.prio = reader.readUnaligned<u8>();
        mdl.getBones()[boneIdx].addDisplay(disp);

        // While with this setup, materials could be XLU and OPA, in
        // practice, they're not.
        const auto& mat = mdl.getMaterials()[disp.matId];
        const bool xlu_mat = mat.isXluPass();
        auto& poly = mdl.getMeshes()[disp.polyId];

        if ((tree == Tree::DrawOpa && xlu_mat) ||
            (tree == Tree::DrawXlu && !xlu_mat)) {
          char warn_msg[1024]{};
          const auto mat_name = ((riistudio::lib3d::Material&)mat).getName();
          snprintf(warn_msg, sizeof(warn_msg),
                   "Material %u \"%s\" is rendered in the %s pass (with mesh "
                   "%u \"%s\"), but is marked as %s.",
                   disp.matId, mat_name.c_str(),
                   xlu_mat ? "Opaue" : "Translucent", disp.polyId,
                   poly.getName().c_str(), !xlu_mat ? "Opaque" : "Translucent");
          reader.warnAt(warn_msg, reader.tell() - 8, reader.tell());
          transaction.callback(kpi::IOMessageClass::Warning,
                               transaction_path + "materials/" + mat_name,
                               warn_msg);
        }
      } break;
      case RenderCommand::NodeDescendence: {
        const auto boneIdx = reader.readUnaligned<u16>();
        // const auto parentMtxIdx =
        reader.readUnaligned<u16>();

        const auto& bone = mdl.getBones()[boneIdx];
        const auto matrixId = bone.matrixId;

        // Hack: Base matrix is first copied to end, first instruction
        if (base_matrix_id == -1) {
          base_matrix_id = matrixId;
        } else {
          auto& drws = mdl.mDrawMatrices;
          if (drws.size() <= matrixId) {
            drws.resize(matrixId + 1);
          }
          // TODO: Start at 100?
          drws[matrixId].mWeights.emplace_back(boneIdx, 1.0f);
        }
        // printf("BoneIdx: %u (MatrixIdx: %u), ParentMtxIdx: %u\n",
        // (u32)boneIdx,
        //        (u32)matrixId, (u32)parentMtxIdx);
        // printf("\"Matrix %u\" -> \"Matrix %u\" [label=\"parent\"];\n",
        //        (u32)matrixId, (u32)parentMtxIdx);
      } break;
      default:
        // TODO
        break;
      }
    }
    // printf("}\n");
  });
}
static void readTexture(Texture& data, oishii::BinaryReader& reader) {
  const auto start = reader.tell();

  reader.expectMagic<'TEX0', true>();
  reader.read<u32>(); // size
  const u32 revision = reader.read<u32>();
  (void)revision;
  assert(revision == 1 || revision == 3);
  reader.read<s32>(); // BRRES offset
  const s32 ofsTex = reader.read<s32>();
  data.name = readName(reader, start);
  // const u32 flag =
  reader.read<u32>(); // TODO: Paletted textures
  data.dimensions.width = reader.read<u16>();
  data.dimensions.height = reader.read<u16>();
  data.format = reader.read<u32>();
  data.mipLevel = reader.read<u32>();
  data.minLod = reader.read<f32>();
  data.maxLod = reader.read<f32>();
  data.sourcePath = readName(reader, start);
  // Skip user data
  reader.seekSet(start + ofsTex);
  data.resizeData();
  assert(reader.tell() + data.getEncodedSize(true) < reader.endpos());
  memcpy(data.data.data(), reader.getStreamStart() + reader.tell(),
         data.getEncodedSize(true));
  reader.skip(data.getEncodedSize(true));
}

class ArchiveDeserializer {
public:
  std::string canRead(const std::string& file,
                      oishii::BinaryReader& reader) const {
    return file.ends_with("brres") ? typeid(Collection).name() : "";
  }
  void read(kpi::IOTransaction& transaction) const {
    auto& node = transaction.node;
    auto& data = transaction.data;

    assert(dynamic_cast<Collection*>(&node) != nullptr);
    Collection& collection = *dynamic_cast<Collection*>(&node);
    oishii::BinaryReader reader(std::move(data));

    // Magic
    reader.read<u32>();
    // byte order
    reader.read<u16>();
    // revision
    reader.read<u16>();
    // filesize
    reader.read<u32>();
    // ofs
    reader.read<u16>();
    // section size
    reader.read<u16>();

    // 'root'
    reader.read<u32>();
    // Length of the section
    reader.read<u32>();
    Dictionary rootDict(reader);

    for (std::size_t i = 1; i < rootDict.mNodes.size(); ++i) {
      const auto& cnode = rootDict.mNodes[i];

      reader.seekSet(cnode.mDataDestination);
      Dictionary cdic(reader);

      // TODO
      if (cnode.mName == "3DModels(NW4R)") {
        for (std::size_t j = 1; j < cdic.mNodes.size(); ++j) {
          const auto& sub = cdic.mNodes[j];

          reader.seekSet(sub.mDataDestination);
          auto& mdl = collection.getModels().add();
          readModel(mdl, reader, transaction,
                    "/" + cnode.mName + "/" + sub.mName + "/");
        }
      } else if (cnode.mName == "Textures(NW4R)") {
        for (std::size_t j = 1; j < cdic.mNodes.size(); ++j) {
          const auto& sub = cdic.mNodes[j];

          reader.seekSet(sub.mDataDestination);
          auto& tex = collection.getTextures().add();
          readTexture(tex, reader);
        }
      } else {
        transaction.callback(kpi::IOMessageClass::Warning, "/" + cnode.mName,
                             "Unsupported folder: " + cnode.mName);

        printf("Unsupported folder: %s\n", cnode.mName.c_str());
      }
    }
  }
};

kpi::Register<ArchiveDeserializer, kpi::Reader> ArchiveInstaller;

} // namespace riistudio::g3d
