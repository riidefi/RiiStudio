#include <core/common.h>
#include <core/kpi/Node.hpp>

#include <oishii/reader/binary_reader.hxx>
#include <oishii/writer/binary_writer.hxx>
#include <oishii/writer/linker.hxx>

#include <string>

#include <plugins/j3d/Scene.hpp>

#include "Sections.hpp"

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

    // SubVeRsion
    writer.write<u32, oishii::EndianSelect::Big>('SVR3');
    for (int i = 0; i < 3; ++i)
      writer.write<u32>(-1);

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
  // assert(len < 30);
  memcpy(dst, "This is padding data to alignment.....", len);
}
class BMD {
public:
  std::string canRead(const std::string& file,
                      oishii::BinaryReader& reader) const {
    return file.ends_with("bmd") || file.ends_with("bdl")
               ? typeid(Collection).name()
               : "";
  }
  bool canWrite(kpi::INode& node) const {
    return dynamic_cast<Collection*>(&node) != nullptr;
  }

  void
  processModelForWrite(j3d::Collection& collection, j3d::Model& model,
                       const std::map<std::string, u32>& texNameMap) const {
    auto& texCache = model.mTexCache;
    auto& matCache = model.mMatCache;
    texCache.clear();
    matCache.clear();

    for (auto& mat : model.getMaterials()) {
      for (int i = 0; i < mat.samplers.size(); ++i) {
        auto& samp = mat.samplers[i];
        if (samp.get() == nullptr)
          continue;
        auto* slow = dynamic_cast<Material::J3DSamplerData*>(samp.get());
        if (slow == nullptr)
          printf("Invalid sampler referring to %s\n", samp->mTexture.c_str());
        assert(slow != nullptr);

        assert(!samp->mTexture.empty());
        if (!samp->mTexture.empty()) {
          const auto btiId = texNameMap.at(samp->mTexture);
          Tex tmp(collection.getTextures()[btiId], *samp.get());
          tmp.btiId = btiId;

          auto found = std::find(texCache.begin(), texCache.end(), tmp);
          if (found == texCache.end()) {
            texCache.push_back(tmp);
            slow->btiId = texCache.size() - 1;
          } else {
            slow->btiId = found - texCache.begin();
          }
        }
      }
      matCache.propogate(mat);
    }
  }

  // Recompute cache
  void processCollectionForWrite(j3d::Collection& collection) const {
    std::map<std::string, u32> texNameMap;
    for (int i = 0; i < collection.getTextures().size(); ++i) {
      texNameMap[collection.getTextures()[i].getName()] = i;
    }

    for (auto& model : collection.getModels()) {
      processModelForWrite(collection, model, texNameMap);
    }
  }

  void write(kpi::INode& node, oishii::Writer& writer) const {
    assert(dynamic_cast<Collection*>(&node) != nullptr);
    Collection& collection = *dynamic_cast<Collection*>(&node);

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

  void read(kpi::IOTransaction& transaction) const {
    auto& node = transaction.node;
    auto& data = transaction.data;

    assert(dynamic_cast<Collection*>(&node) != nullptr);
    auto& collection = *dynamic_cast<Collection*>(&node);
    auto& mdl = collection.getModels().add();

    oishii::BinaryReader reader(std::move(data));

    BMDOutputContext ctx{mdl, collection, reader, transaction};

    reader.setEndian(true);
    const auto magic = reader.read<u32>();
    if (magic == 'J3D2') {
      // Big endian
    } else if (magic == '2D3J') {
      reader.setEndian(false);
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
          assert(mat.texMatrices[i]->isIdentity());
          tg.setMatrixIndex(i);
        }
      }
    }

    // Read TEX1
    readTEX1(ctx);

    // Read INF1
    readINF1(ctx);

    // Read MDL3
  }
};

kpi::Register<BMD, kpi::Reader | kpi::Writer> BMDInstaller;

} // namespace riistudio::j3d
