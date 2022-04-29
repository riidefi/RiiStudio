#include "Common.hpp"
#include <core/common.h>
#include <core/kpi/Plugins.hpp>
#include <librii/g3d/io/DictWriteIO.hpp>
#include <librii/g3d/io/TevIO.hpp>
#include <librii/gpu/DLBuilder.hpp>
#include <librii/gpu/DLPixShader.hpp>
#include <librii/gpu/GPUMaterial.hpp>
#include <librii/gx.h>
#include <plugins/g3d/collection.hpp>
#include <plugins/g3d/util/NameTable.hpp>

namespace librii::g3d {
using namespace bad;
}

namespace riistudio::g3d {

template <typename T, bool HasMinimum, bool HasDivisor,
          librii::gx::VertexBufferKind kind>
std::string
readGenericBuffer(GenericBuffer<T, HasMinimum, HasDivisor, kind>& out,
                  oishii::BinaryReader& reader) {
  const auto start = reader.tell();
  out.mEntries.clear();

  reader.read<u32>(); // size
  reader.read<u32>(); // mdl0 offset
  const auto startOfs = reader.read<s32>();
  out.mName = readName(reader, start);
  out.mId = reader.read<u32>();
  out.mQuantize.mComp = librii::gx::VertexComponentCount(
      static_cast<librii::gx::VertexComponentCount::Normal>(
          reader.read<u32>()));
  out.mQuantize.mType = librii::gx::VertexBufferType(
      static_cast<librii::gx::VertexBufferType::Color>(reader.read<u32>()));
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
      librii::gx::computeComponentCount(kind, out.mQuantize.mComp);
  if (kind == librii::gx::VertexBufferKind::normal) {
    switch (out.mQuantize.mType.generic) {
    case librii::gx::VertexBufferType::Generic::s8: {
      if ((int)out.mQuantize.divisor != 6) {
        return "Invalid divisor for S8 normal data";
      }
      break;
    }
    case librii::gx::VertexBufferType::Generic::s16: {
      if ((int)out.mQuantize.divisor != 14) {
        return "Invalid divisor for S16 normal data";
      }
      break;
    }
    case librii::gx::VertexBufferType::Generic::u8:
      return "Invalid quantization for normal data: U8";
    case librii::gx::VertexBufferType::Generic::u16:
      return "Invalid quantization for normal data: U16";
    case librii::gx::VertexBufferType::Generic::f32:
      if ((int)out.mQuantize.divisor != 0) {
        return "Misleading divisor for F32 normal data";
      }
      break;
    }
  }

  reader.seekSet(start + startOfs);
  // TODO: Recompute bounds
  for (auto& entry : out.mEntries) {
    entry = librii::gx::readComponents<T>(reader, out.mQuantize.mType,
                                          nComponents, out.mQuantize.divisor);
  }

  return ""; // Valid
}

bool readMaterial(G3dMaterialData& mat, oishii::BinaryReader& reader) {
  const auto start = reader.tell();

  reader.read<u32>(); // size
  reader.read<s32>(); // mdl offset
  mat.name = readName(reader, start);
  mat.id = reader.read<u32>();
  mat.flag = reader.read<u32>();

  // Gen info
  mat.texGens.resize(reader.read<u8>());
  auto nColorChan = reader.read<u8>();
  if (nColorChan >= 2)
    nColorChan = 2;
  mat.mStages.resize(reader.read<u8>());
  mat.indirectStages.resize(reader.read<u8>());
  mat.cullMode = static_cast<librii::gx::CullMode>(reader.read<u32>());

  // Misc
  mat.earlyZComparison = reader.read<u8>();
  mat.lightSetIndex = reader.read<s8>();
  mat.fogIndex = reader.read<s8>();
  reader.read<u8>();
  const auto indSt = reader.tell();
  for (u8 i = 0; i < mat.indirectStages.size(); ++i) {
    librii::g3d::G3dIndConfig cfg;
    cfg.method = static_cast<librii::g3d::G3dIndMethod>(reader.read<u8>());
    cfg.normalMapLightRef = reader.peekAt<s8>(4);
    mat.indConfig.push_back(cfg);
  }
  reader.seekSet(indSt + 4 + 4);

  const auto ofsTev = reader.read<s32>();
  const auto nTex = reader.read<u32>();
  if (nTex > 8) {
    return false;
  }
  const auto [ofsSamplers, ofsFur, ofsUserData, ofsDisplayLists] =
      reader.readX<s32, 4>();
  if (ofsFur || ofsUserData)
    printf("Warning: Material %s uses Fur or UserData which is unsupported!\n",
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
    libcube::GCMaterialData::TexMatrix mtx;
    mtx.scale << reader;
    mtx.rotate = glm::radians(reader.read<f32>());
    mtx.translate << reader;
    mat.texMatrices.push_back(mtx);
  }
  reader.seekSet(start + 0x250);
  for (u8 i = 0; i < nTex; ++i) {
    auto* mtx = &mat.texMatrices[i];

    mtx->camIdx = reader.read<s8>();
    mtx->lightIdx = reader.read<s8>();
    // printf("[read] CamIDX: %i, LIDX:%i\n", (signed)mtx->camIdx,
    //        (signed)mtx->lightIdx);
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
      mtx->method =
          libcube::GCMaterialData::CommonMappingMethod::EnvironmentLightMapping;
      break;
    case 4:
      mtx->method = libcube::GCMaterialData::CommonMappingMethod::
          EnvironmentSpecularMapping;
      break;
    // EGG
    case 5:
      mtx->method =
          libcube::GCMaterialData::CommonMappingMethod::ManualProjectionMapping;
      break;
    }

    reader.read<u8>(); // effect mtx flag
    // G3D effectmatrix is 3x4. J3D is 4x4
    reader.skip(3 * 4 * sizeof(f32));
    // for (auto& f : mtx->effectMatrix)
    //   f = reader.read<f32>();
  }
  // reader.seek((8 - nTex)* (4 + (4 * 4 * sizeof(f32))));
  reader.seekSet(start + 0x3f0);
  mat.chanData.resize(0);
  for (u8 i = 0; i < nColorChan; ++i) {
    // skip runtime flag
    const auto flag = reader.read<u32>();
    (void)flag;
    librii::gx::Color matClr, ambClr;
    matClr.r = reader.read<u8>();
    matClr.g = reader.read<u8>();
    matClr.b = reader.read<u8>();
    matClr.a = reader.read<u8>();

    ambClr.r = reader.read<u8>();
    ambClr.g = reader.read<u8>();
    ambClr.b = reader.read<u8>();
    ambClr.a = reader.read<u8>();

    mat.chanData.push_back({matClr, ambClr});
    librii::gpu::LitChannel tmp;
    // Color
    tmp.hex = reader.read<u32>();
    mat.colorChanControls.push_back(tmp);
    // Alpha
    tmp.hex = reader.read<u32>();
    mat.colorChanControls.push_back(tmp);
  }

  // TEV
  auto tev_addr = start + ofsTev;
  librii::g3d::ReadTev(mat, reader, tev_addr);

  // Samplers
  reader.seekSet(start + ofsSamplers);
  if (mat.texGens.size() != nTex) {
    return false;
  }
  for (u8 i = 0; i < nTex; ++i) {
    libcube::GCMaterialData::SamplerData sampler;

    const auto sampStart = reader.tell();
    sampler.mTexture = readName(reader, sampStart);
    sampler.mPalette = readName(reader, sampStart);
    reader.skip(8);     // skip runtime pointers
    reader.read<u32>(); // skip tex id for now
    reader.read<u32>(); // skip tlut id for now
    sampler.mWrapU =
        static_cast<librii::gx::TextureWrapMode>(reader.read<u32>());
    sampler.mWrapV =
        static_cast<librii::gx::TextureWrapMode>(reader.read<u32>());
    sampler.mMinFilter =
        static_cast<librii::gx::TextureFilter>(reader.read<u32>());
    sampler.mMagFilter =
        static_cast<librii::gx::TextureFilter>(reader.read<u32>());
    sampler.mLodBias = reader.read<f32>();
    sampler.mMaxAniso =
        static_cast<librii::gx::AnisotropyLevel>(reader.read<u32>());
    sampler.bBiasClamp = reader.read<u8>();
    sampler.bEdgeLod = reader.read<u8>();
    reader.skip(2);
    mat.samplers.push_back(std::move(sampler));
  }

  // Display Lists
  reader.seekSet(start + ofsDisplayLists);
  {
    librii::gpu::QDisplayListMaterialHandler matHandler(mat);

    // Pixel data
    librii::gpu::RunDisplayList(reader, matHandler, 32);
    mat.alphaCompare = matHandler.mGpuMat.mPixel.mAlphaCompare;
    mat.zMode = matHandler.mGpuMat.mPixel.mZMode;
    mat.blendMode = matHandler.mGpuMat.mPixel.mBlendMode;
    // TODO: Dst alpha

    // Uniform data
    librii::gpu::RunDisplayList(reader, matHandler, 128);
    mat.tevColors[0] = {0xff, 0xff, 0xff, 0xff};
    for (int i = 0; i < 3; ++i) {
      mat.tevColors[i + 1] = matHandler.mGpuMat.mShaderColor.Registers[i + 1];
    }
    for (int i = 0; i < 4; ++i) {
      mat.tevKonstColors[i] = matHandler.mGpuMat.mShaderColor.Konstants[i];
    }

    // Indirect data
    librii::gpu::RunDisplayList(reader, matHandler, 64);
    for (u8 i = 0; i < mat.indirectStages.size(); ++i) {
      const auto& curScale =
          matHandler.mGpuMat.mIndirect.mIndTexScales[i > 1 ? i - 2 : i];
      mat.indirectStages[i].scale = librii::gx::IndirectTextureScalePair{
          static_cast<librii::gx::IndirectTextureScalePair::Selection>(
              curScale.ss0),
          static_cast<librii::gx::IndirectTextureScalePair::Selection>(
              curScale.ss1)};

      mat.mIndMatrices.push_back(matHandler.mGpuMat.mIndirect.mIndMatrices[i]);
    }

    const std::array<u32, 9> texGenDlSizes{
        0,        // 0
        32,       // 1
        64,  64,  // 2, 3
        96,  96,  // 4, 5
        128, 128, // 6, 7
        160       // 8
    };
    librii::gpu::RunDisplayList(reader, matHandler,
                                texGenDlSizes[mat.texGens.size()]);
    for (u8 i = 0; i < mat.texGens.size(); ++i) {
      mat.texGens[i] = matHandler.mGpuMat.mTexture[i];
      mat.texMatrices[i].projection = mat.texGens[i].func;
    }
  }

  return true;
}

bool readBone(librii::g3d::BoneData& bone, oishii::BinaryReader& reader,
              u32 bone_id, s32 data_destination) {
  reader.skip(8); // skip size and mdl offset
  bone.mName = readName(reader, data_destination);

  const u32 id = reader.read<u32>();
  if (id != bone_id) {
    return false;
  }
  bone.matrixId = reader.read<u32>();
  const auto bone_flag = reader.read<u32>();
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
    oishii::Jump<oishii::Whence::Set>(reader, data_destination + ofs + 12);
    return static_cast<s32>(reader.read<u32>());
  };
  bone.mParent = readHierarchyElement();
  reader.skip(12); // Skip sibling and child links -- we recompute it all
  reader.skip(2 * ((3 * 4) * sizeof(f32))); // skip matrices

  setFromFlag(bone, bone_flag);
  return true;
}

void ReadModelInfo(oishii::BinaryReader& reader,
                   librii::g3d::G3DModelDataData& mdl) {
  const auto infoPos = reader.tell();
  reader.skip(8); // Ignore size, ofsMode
  mdl.mScalingRule = static_cast<librii::g3d::ScalingRule>(reader.read<u32>());
  mdl.mTexMtxMode =
      static_cast<librii::g3d::TextureMatrixMode>(reader.read<u32>());

  reader.readX<u32, 2>(); // number of vertices, number of triangles
  mdl.sourceLocation = readName(reader, infoPos);
  reader.read<u32>(); // number of view matrices

  // const auto [bMtxArray, bTexMtxArray, bBoundVolume] =
  reader.readX<u8, 3>();
  mdl.mEvpMtxMode =
      static_cast<librii::g3d::EnvelopeMatrixMode>(reader.read<u8>());

  // const s32 ofsBoneTable =
  reader.read<s32>();

  mdl.aabb.min << reader;
  mdl.aabb.max << reader;
}

static void ReadRenderTree(const librii::g3d::DictionaryNode& dnode,
                           oishii::BinaryReader& reader, g3d::Model& mdl,
                           kpi::LightIOTransaction& transaction,
                           const std::string& transaction_path) {
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
      auto boneIdx = reader.readUnaligned<u16>();
      disp.prio = reader.readUnaligned<u8>();

      if (boneIdx > mdl.getBones().size()) {
        transaction.callback(kpi::IOMessageClass::Error, transaction_path,
                             "Invalid bone index in render command");
        boneIdx = 0;
        transaction.state = kpi::TransactionState::Failure;
      }

      if (disp.matId > mdl.getMeshes().size()) {
        transaction.callback(kpi::IOMessageClass::Error, transaction_path,
                             "Invalid material index in render command");
        disp.matId = 0;
        transaction.state = kpi::TransactionState::Failure;
      }

      if (disp.polyId > mdl.getMeshes().size()) {
        transaction.callback(kpi::IOMessageClass::Error, transaction_path,
                             "Invalid mesh index in render command");
        disp.polyId = 0;
        transaction.state = kpi::TransactionState::Failure;
      }

      mdl.getBones()[boneIdx].addDisplay(disp);

      // While with this setup, materials could be XLU and OPA, in
      // practice, they're not.
      auto& mat = mdl.getMaterials()[disp.matId];
      const bool xlu_mat = mat.flag & 0x80000000;
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
      mat.setXluPass(tree == Tree::DrawXlu);
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
}

void readModel(Model& mdl, oishii::BinaryReader& reader,
               kpi::LightIOTransaction& transaction,
               const std::string& transaction_path) {
  const auto start = reader.tell();
  bool isValid = true;

  if (!reader.expectMagic<'MDL0', false>()) {
    return;
  }

  MAYBE_UNUSED const u32 fileSize = reader.read<u32>();
  const u32 revision = reader.read<u32>();
  if (revision != 11) {
    transaction.callback(kpi::IOMessageClass::Error, transaction_path,
                         "MDL0 is version " + std::to_string(revision) +
                             ". Only MDL0 version 11 is supported.");
    transaction.state = kpi::TransactionState::Failure;
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

  mdl.mName = readName(reader, start);

  ReadModelInfo(reader, mdl);

  auto readDict = [&](u32 xofs, auto handler) {
    if (xofs) {
      reader.seekSet(start + xofs);
      librii::g3d::Dictionary _dict(reader);
      for (std::size_t i = 1; i < _dict.mNodes.size(); ++i) {
        const auto& dnode = _dict.mNodes[i];
        assert(dnode.mDataDestination);
        reader.seekSet(dnode.mDataDestination);
        handler(dnode);
      }
    }
  };

  u32 bone_id = 0;
  readDict(secOfs.ofsBones, [&](const librii::g3d::DictionaryNode& dnode) {
    auto& bone = mdl.getBones().add();
    if (!readBone(bone, reader, bone_id, dnode.mDataDestination)) {
      printf("Failed to read bone %s\n", dnode.mName.c_str());
    }
    bone_id++;
  });
  // Compute children
  for (int i = 0; i < mdl.getBones().size(); ++i) {
    if (const auto parent_id = mdl.getBones()[i].mParent; parent_id >= 0) {
      if (parent_id >= mdl.getBones().size()) {
        printf("Invalidly large parent index..\n");
        break;
      }
      mdl.getBones()[parent_id].mChildren.push_back(i);
    }
  }
  readDict(secOfs.ofsBuffers.position,
           [&](const librii::g3d::DictionaryNode& dnode) {
             auto err = readGenericBuffer(mdl.getBuf_Pos().add(), reader);
             if (err.size()) {
               transaction.callback(kpi::IOMessageClass::Error,
                                    transaction_path, err);
               transaction.state = kpi::TransactionState::Failure;
             }
           });
  readDict(secOfs.ofsBuffers.normal,
           [&](const librii::g3d::DictionaryNode& dnode) {
             auto err = readGenericBuffer(mdl.getBuf_Nrm().add(), reader);
             if (err.size()) {
               transaction.callback(kpi::IOMessageClass::Error,
                                    transaction_path, err);
               transaction.state = kpi::TransactionState::Failure;
             }
           });
  readDict(secOfs.ofsBuffers.color,
           [&](const librii::g3d::DictionaryNode& dnode) {
             auto err = readGenericBuffer(mdl.getBuf_Clr().add(), reader);
             if (err.size()) {
               transaction.callback(kpi::IOMessageClass::Error,
                                    transaction_path, err);
               transaction.state = kpi::TransactionState::Failure;
             }
           });
  readDict(secOfs.ofsBuffers.uv, [&](const librii::g3d::DictionaryNode& dnode) {
    auto err = readGenericBuffer(mdl.getBuf_Uv().add(), reader);
    if (err.size()) {
      transaction.callback(kpi::IOMessageClass::Error, transaction_path, err);
      transaction.state = kpi::TransactionState::Failure;
    }
  });

  if (transaction.state == kpi::TransactionState::Failure)
    return;

  // TODO: Fur

  readDict(secOfs.ofsMaterials, [&](const librii::g3d::DictionaryNode& dnode) {
    auto& mat = mdl.getMaterials().add();
    const bool ok = readMaterial(mat, reader);

    if (!ok) {
      printf("Failed to read material %s\n", dnode.mName.c_str());
    }
  });

  readDict(secOfs.ofsMeshes, [&](const librii::g3d::DictionaryNode& dnode) {
    auto& poly = mdl.getMeshes().add();
    const auto start = reader.tell();

    isValid &= reader.read<u32>() != 0; // size
    isValid &= reader.read<s32>() < 0;  // mdl offset

    poly.mCurrentMatrix = reader.read<s32>();
    reader.skip(12); // cache

    DlHandle primitiveSetup(reader);
    DlHandle primitiveData(reader);

    poly.mVertexDescriptor.mBitfield = reader.read<u32>();
    const u32 flag = reader.read<u32>();
    poly.currentMatrixEmbedded = flag & 1;
    if (poly.currentMatrixEmbedded) {
      // TODO (should be caught later)
    }
    poly.visible = !(flag & 2);

    poly.mName = readName(reader, start);
    poly.mId = reader.read<u32>();
    // TODO: Verify / cache
    isValid &= reader.read<u32>() > 0; // nVert
    isValid &= reader.read<u32>() > 0; // nPoly

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
    isValid &= reader.read<s32>() == -1; // fur
    reader.read<s32>();                  // matrix usage

    primitiveSetup.seekTo(reader);
    librii::gpu::QDisplayListVertexSetupHandler vcdHandler;
    librii::gpu::RunDisplayList(reader, vcdHandler, primitiveSetup.buf_size);

    for (u32 i = 0; i < (u32)librii::gx::VertexAttribute::Max; ++i) {
      if (poly.mVertexDescriptor.mBitfield & (1 << i)) {
        if (i == 0) {
          transaction.callback(kpi::IOMessageClass::Error, transaction_path,
                               "Unsuported attribute");
          transaction.state = kpi::TransactionState::Failure;
          return;
        }
        const auto stat = vcdHandler.mGpuMesh.VCD.GetVertexArrayStatus(
            i - (u32)librii::gx::VertexAttribute::Position);
        const auto att = static_cast<librii::gx::VertexAttributeType>(stat);
        if (att == librii::gx::VertexAttributeType::None) {
          transaction.callback(kpi::IOMessageClass::Error, transaction_path,
                               "att == librii::gx::VertexAttributeType::None");
          poly.mVertexDescriptor.mBitfield ^= (1 << i);
          // transaction.state = kpi::TransactionState::Failure;
          // return;
        }
        poly.mVertexDescriptor.mAttributes[(librii::gx::VertexAttribute)i] =
            att;
      }
    }
    struct QDisplayListMeshHandler final
        : public librii::gpu::QDisplayListHandler {
      void onCommandDraw(oishii::BinaryReader& reader,
                         librii::gx::PrimitiveType type, u16 nverts) override {
        if (mErr)
          return;

        if (mPoly.mMatrixPrimitives.empty())
          mPoly.mMatrixPrimitives.push_back(MatrixPrimitive{});
        auto& prim = mPoly.mMatrixPrimitives.back().mPrimitives.emplace_back(
            librii::gx::IndexedPrimitive{});
        prim.mType = type;
        prim.mVertices.resize(nverts);
        for (auto& vert : prim.mVertices) {
          for (u32 i = 0;
               i < static_cast<u32>(librii::gx::VertexAttribute::Max); ++i) {
            if (mPoly.mVertexDescriptor.mBitfield & (1 << i)) {
              const auto attr = static_cast<librii::gx::VertexAttribute>(i);
              switch (mPoly.mVertexDescriptor.mAttributes[attr]) {
              case librii::gx::VertexAttributeType::Direct:
                mErr = true;
                return;
              case librii::gx::VertexAttributeType::None:
                break;
              case librii::gx::VertexAttributeType::Byte:
                vert[attr] = reader.readUnaligned<u8>();
                break;
              case librii::gx::VertexAttributeType::Short:
                vert[attr] = reader.readUnaligned<u16>();
                break;
              }
            }
          }
        }
      }
      QDisplayListMeshHandler(Polygon& poly) : mPoly(poly) {}
      bool mErr = false;
      Polygon& mPoly;
    } meshHandler(poly);
    primitiveData.seekTo(reader);
    librii::gpu::RunDisplayList(reader, meshHandler, primitiveData.buf_size);
    if (meshHandler.mErr) {
      transaction.callback(kpi::IOMessageClass::Warning, transaction_path,
                           "Mesh unsupported.");
      transaction.state = kpi::TransactionState::Failure;
    }
  });

  if (transaction.state == kpi::TransactionState::Failure)
    return;

  readDict(secOfs.ofsRenderTree, [&](const librii::g3d::DictionaryNode& dnode) {
    ReadRenderTree(dnode, reader, mdl, transaction, transaction_path);
  });

  if (!isValid && mdl.getBones().size() > 1) {
    transaction.callback(
        kpi::IOMessageClass::Error, transaction_path,
        "BRRES file was created with BrawlBox and is invalid. It is "
        "recommended you create BRRES files here by dropping a DAE/FBX file.");
    //
    transaction.state = kpi::TransactionState::FailureToSave;
  } else if (!isValid) {
    transaction.callback(kpi::IOMessageClass::Warning, transaction_path,
                         "Note: BRRES file was saved with BrawlBox. Certain "
                         "materials may flicker during ghost replays.");
  } else if (mdl.getBones().size() > 1) {
    transaction.callback(kpi::IOMessageClass::Error, transaction_path,
                         "Rigging support is not fully tested. "
                         "Rejecting file to avoid potential corruption.");
    transaction.state = kpi::TransactionState::FailureToSave;
  }
}

} // namespace riistudio::g3d
