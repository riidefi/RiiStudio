#include <core/common.h>
#include <core/kpi/Node2.hpp>

#include <oishii/reader/binary_reader.hxx>
#include <oishii/writer/binary_writer.hxx>
#include <oishii/writer/linker.hxx>

#include <string>

#include <plugins/j3d/Scene.hpp>

#include "Sections.hpp"

#include <librii/gx/validate/MaterialValidate.hpp>

#include <core/util/timestamp.hpp>

bool gTestMode = false;

namespace riistudio::j3d {

using namespace libcube;

struct BMDFile : public oishii::Node {
  static const char* getNameId() { return "JSystem Binary Model Data"; }

  Result write(oishii::Writer& writer) const noexcept {
    // Hack
    writer.write<u32, oishii::EndianSelect::Big>('J3D2');
    writer.write<u32, oishii::EndianSelect::Big>(bBDL ? 'bdl4' : 'bmd3');

    // Filesize
    writer.writeLink<s32>(oishii::Link{
        oishii::Hook(*this), oishii::Hook(*this, oishii::Hook::EndOfChildren)});

    // 8 sections
    writer.write<u32>(bBDL ? 9 : 8);

    if (gTestMode) {
      // SubVeRsion
      writer.write<u32, oishii::EndianSelect::Big>('SVR3');
      for (int i = 0; i < 3; ++i)
        writer.write<u32>(-1);
    } else {
      std::string author = VERSION_SHORT;
      for (char c : author)
        writer.write<char>(c);
    }

    return {};
  }
  Result gatherChildren(oishii::Node::NodeDelegate& ctx) const {
    BMDExportContext exp{mCollection->getModels()[0], *mCollection};

    auto addNode = [&](std::unique_ptr<oishii::Node> node) {
      node->getLinkingRestriction().alignment = 32;
      ctx.addNode(std::move(node));
    };

    addNode(makeINF1Node(exp));
    addNode(makeVTX1Node(exp));
    addNode(makeEVP1Node(exp));
    addNode(makeDRW1Node(exp));
    addNode(makeJNT1Node(exp));
    addNode(makeSHP1Node(exp));
    addNode(makeMAT3Node(exp));
    if (bBDL)
      addNode(makeMDL3Node(exp));
    addNode(makeTEX1Node(exp));
    return {};
  }

  j3d::Collection* mCollection;
  bool bBDL = true;
  bool bMimic = true;
};
void BMD_Pad(char* dst, u32 len) {
  const char* pad_str = "This is padding data to alignment.....";
  if (!gTestMode) {
    pad_str = RII_TIME_STAMP;
  }

  if (len < strlen(pad_str))
    memcpy(dst, pad_str, len);
  else
    memset(dst, 0, len);
}

void processModelForWrite(j3d::Collection& collection, j3d::Model& model,
                          const std::map<std::string, u32>& texNameMap) {
  auto& texCache = model.mTexCache;
  auto& matCache = model.mMatCache;
  texCache.clear();
  matCache.clear();

  for (auto& mat : model.getMaterials()) {
    for (int i = 0; i < mat.samplers.size(); ++i) {
      auto* samp = &mat.samplers[i];

      assert(!samp->mTexture.empty());
      if (!samp->mTexture.empty()) {
        const auto btiId = texNameMap.at(samp->mTexture);
        Tex tmp(collection.getTextures()[btiId], *samp);
        tmp.btiId = btiId;

        auto found = std::find(texCache.begin(), texCache.end(), tmp);
        if (found == texCache.end()) {
          texCache.push_back(tmp);
          samp->btiId = texCache.size() - 1;
        } else {
          samp->btiId = found - texCache.begin();
        }
      }
    }
    matCache.propogate(mat);
  }
}

// Recompute cache
void processCollectionForWrite(j3d::Collection& collection) {
  std::map<std::string, u32> texNameMap;
  for (int i = 0; i < collection.getTextures().size(); ++i) {
    texNameMap[collection.getTextures()[i].getName()] = i;
  }

  for (auto& model : collection.getModels()) {
    processModelForWrite(collection, model, texNameMap);
  }
}

void WriteBMD(riistudio::j3d::Collection& collection, oishii::Writer& writer) {
  oishii::Linker linker;

  auto bmd = std::make_unique<BMDFile>();
  bmd->bBDL = collection.getModels()[0].isBDL;
  bmd->bMimic = true;
  bmd->mCollection = &collection;

  linker.mUserPad = &BMD_Pad;
  writer.mUserPad = &BMD_Pad;

  processCollectionForWrite(collection);

  // writer.add_bp(0x37b2c, 4);

  linker.gather(std::move(bmd), "");
  linker.write(writer);
}

void ReadBMD(riistudio::j3d::Model& mdl, riistudio::j3d::Collection& collection,
             oishii::BinaryReader& reader,
             kpi::LightIOTransaction& transaction) {
  BMDOutputContext ctx{mdl, collection, reader, transaction};

  reader.setEndian(std::endian::big);
  const auto magic = reader.read<u32>();
  if (magic == 'J3D2') {
    // Big endian
  } else if (magic == '2D3J') {
    reader.setEndian(std::endian::little);
  } else {
    // Invalid file
    return;
  }

  mdl.isBDL = false;

  u32 bmdVer = reader.read<u32>();
  if (bmdVer == 'bmd3') {
  } else if (bmdVer == 'bdl4') {
#ifndef BUILD_DIST
    // mdl.isBDL = true;
#endif
  } else {
    reader.signalInvalidityLast<u32, oishii::MagicInvalidity<'bmd3'>>();
    // error = true;
    return;
  }

  // TODO: Validate file size.
  // const auto fileSize =
  reader.read<u32>();
  const auto sec_count = reader.read<u32>();

  // Skip SVR3 header
  reader.seek<oishii::Whence::Current>(16);

  // Scan the file
  ctx.mSections.clear();
  for (u32 i = 0; i < sec_count; ++i) {
    const u32 secType = ctx.reader.read<u32>();
    const u32 secSize = ctx.reader.read<u32>();

    {
      oishii::JumpOut g(ctx.reader, secSize - 8);
      switch (secType) {
      case 'INF1':
      case 'VTX1':
      case 'EVP1':
      case 'DRW1':
      case 'JNT1':
      case 'SHP1':
      case 'MAT3':
      case 'MDL3':
      case 'TEX1':
        ctx.mSections[secType] = {ctx.reader.tell(), secSize};
        break;
      default:
        ctx.reader.warnAt("Unexpected section type.", ctx.reader.tell() - 8,
                          ctx.reader.tell() - 4);
        break;
      }
    }
  }
  // Read vertex data
  readVTX1(ctx);

  // Read joints
  readJNT1(ctx);

  // Read envelopes and draw matrices
  readEVP1DRW1(ctx);

  // Read shapes
  readSHP1(ctx);

  for (const auto& e : ctx.mVertexBufferMaxIndices) {
    switch (e.first) {
    case gx::VertexBufferAttribute::Position:
      if (ctx.mdl.mBufs.pos.mData.size() != e.second + 1) {
        DebugReport(
            "The position vertex buffer currently has %u greedily-claimed "
            "entries due to 32B padding; %u are used.\n",
            (u32)ctx.mdl.mBufs.pos.mData.size(), e.second + 1);
        ctx.mdl.mBufs.pos.mData.resize(e.second + 1);
      }
      break;
    case gx::VertexBufferAttribute::Color0:
    case gx::VertexBufferAttribute::Color1: {
      auto& buf =
          ctx.mdl.mBufs
              .color[(int)e.first - (int)gx::VertexBufferAttribute::Color0];
      if (buf.mData.size() != e.second + 1) {
        DebugReport(
            "The color buffer currently has %u greedily-claimed entries "
            "due to 32B padding; %u are used.\n",
            (u32)buf.mData.size(), e.second + 1);
        buf.mData.resize(e.second + 1);
      }
      break;
    }
    case gx::VertexBufferAttribute::Normal: {
      auto& buf = ctx.mdl.mBufs.norm;
      if (buf.mData.size() != e.second + 1) {
        DebugReport(
            "The normal buffer currently has %u greedily-claimed entries "
            "due to 32B padding; %u are used.\n",
            (u32)buf.mData.size(), e.second + 1);
        buf.mData.resize(e.second + 1);
      }
      break;
    }
    case gx::VertexBufferAttribute::TexCoord0:
    case gx::VertexBufferAttribute::TexCoord1:
    case gx::VertexBufferAttribute::TexCoord2:
    case gx::VertexBufferAttribute::TexCoord3:
    case gx::VertexBufferAttribute::TexCoord4:
    case gx::VertexBufferAttribute::TexCoord5:
    case gx::VertexBufferAttribute::TexCoord6:
    case gx::VertexBufferAttribute::TexCoord7: {
      auto& buf =
          ctx.mdl.mBufs
              .uv[(int)e.first - (int)gx::VertexBufferAttribute::TexCoord0];
      if (buf.mData.size() != e.second + 1) {
        DebugReport(
            "The UV buffer currently has %u greedily-claimed entries due "
            "to 32B padding; %u are used.\n",
            (u32)buf.mData.size(), e.second + 1);
        buf.mData.resize(e.second + 1);
      }
    }
    default:
      break; // TODO
    }
  }

  // Read materials
  readMAT3(ctx);

  // fixup identity matrix optimization for editor
  for (auto& mat : ctx.mdl.getMaterials()) {
    if (mat.texGens.size() != mat.texMatrices.size())
      continue;

    // Compute a used bitmap. 8 max texgens, 10 max texmatrices
    u32 used = 0;
    for (int i = 0; i < mat.texGens.size(); ++i) {
      if (auto idx = mat.texGens[i].getMatrixIndex(); idx > 0)
        used |= (1 << idx);
    }

    for (int i = 0; i < mat.texGens.size(); ++i) {
      auto& tg = mat.texGens[i];

      if (tg.isIdentityMatrix() && (used & (1 << i)) == 0) {
        assert(mat.texMatrices[i].isIdentity());
        tg.setMatrixIndex(i);
      }
    }

    librii::gx::expandSharedTexGens(mat);
  }

  // Read TEX1
  readTEX1(ctx);

  // Read INF1
  readINF1(ctx);

  // Read MDL3
}

} // namespace riistudio::j3d
