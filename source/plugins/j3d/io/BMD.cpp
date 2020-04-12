#include "BMD.hpp"

#include <core/common.h>
#include <core/kpi/Node.hpp>

#include <oishii/reader/binary_reader.hxx>
#include <oishii/v2/writer/binary_writer.hxx>
#include <oishii/v2/writer/linker.hxx>

#include <string>

#include <plugins/j3d/Scene.hpp>

#include "Sections.hpp"

namespace riistudio::j3d {

using namespace libcube;

inline bool ends_with(const std::string &value, const std::string &ending) {
  return ending.size() <= value.size() &&
         std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

struct BMDFile : public oishii::v2::Node {
  static const char *getNameId() { return "JSystem Binary Model Data"; }

  Result write(oishii::v2::Writer &writer) const noexcept {
    // Hack
    writer.write<u32, oishii::EndianSelect::Big>('J3D2');
    writer.write<u32, oishii::EndianSelect::Big>(bBDL ? 'bdl4' : 'bmd3');

    // Filesize
    writer.writeLink<s32>(oishii::v2::Link{
        oishii::v2::Hook(*this),
        oishii::v2::Hook(*this, oishii::v2::Hook::EndOfChildren)});

    // 8 sections
    writer.write<u32>(bBDL ? 9 : 8);

    // SubVeRsion
    writer.write<u32, oishii::EndianSelect::Big>('SVR3');
    for (int i = 0; i < 3; ++i)
      writer.write<u32>(-1);

    return {};
  }
  Result gatherChildren(oishii::v2::Node::NodeDelegate &ctx) const {
    BMDExportContext exp{mCollection.getModel(0), mCollection};

    auto addNode = [&](std::unique_ptr<oishii::v2::Node> node) {
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
    addNode(makeTEX1Node(exp));
    return {};
  }

  CollectionAccessor mCollection = CollectionAccessor{nullptr};
  bool bBDL = true;
  bool bMimic = true;
};
void BMD_Pad(char *dst, u32 len) {
  // assert(len < 30);
  memcpy(dst, "This is padding data to alignment.....", len);
}
class BMD {
public:
  std::string canRead(const std::string &file,
                      oishii::BinaryReader &reader) const {
    return ends_with(file, "bmd") || ends_with(file, "bdl")
               ? typeid(Collection).name()
               : "";
  }
  bool canWrite(kpi::IDocumentNode &node) const {
    return dynamic_cast<Collection *>(&node) != nullptr;
  }

  void write(kpi::IDocumentNode &node, oishii::v2::Writer &writer) const {
    assert(dynamic_cast<Collection *>(&node) != nullptr);
    CollectionAccessor collection(&node);

    oishii::v2::Linker linker;

    auto bmd = std::make_unique<BMDFile>();
    bmd->bBDL = false; // collection.bdl;
    bmd->bMimic = true;
    bmd->mCollection = collection;

    linker.mUserPad = &BMD_Pad;
    writer.mUserPad = &BMD_Pad;

    // Recompute cache

    std::map<std::string, u32> texNameMap;
    for (int i = 0; i < collection.getTextures().size(); ++i) {
      texNameMap[collection.getTexture(i).node().getName()] = i;
    }

    for (int m_i = 0; m_i < collection.getModels().size(); ++m_i) {
      auto model = collection.getModel(m_i);
      auto &texCache = model.get().mTexCache;
      auto &matCache = model.get().mMatCache;
      texCache.clear();
      matCache.clear();

      for (int j = 0; j < model.getMaterials().size(); ++j) {
        auto &mat = model.getMaterial(j).get();
        for (int k = 0; k < mat.samplers.size(); ++k) {
          auto &samp = mat.samplers[k];

          auto *slow =
              reinterpret_cast<j3d::MaterialData::J3DSamplerData *>(samp.get());
          assert(slow);

          assert(!samp->mTexture.empty());
          if (!samp->mTexture.empty()) {
            const auto btiId = texNameMap[samp->mTexture];
            Tex tmp(collection.getTexture(btiId).get(), *samp.get());
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

    // writer.add_bp(0x37b2c, 4);

    linker.gather(std::move(bmd), "");
    linker.write(writer);
  }

  void read(kpi::IDocumentNode &node, oishii::BinaryReader &reader) const {
    assert(dynamic_cast<Collection *>(&node) != nullptr);
    CollectionAccessor collection(&node);

    auto mdl = collection.addModel();

    BMDOutputContext ctx{mdl, collection, reader};

    reader.setEndian(true);
    reader.expectMagic<'J3D2'>();

    u32 bmdVer = reader.read<u32>();
    if (bmdVer != 'bmd3' && bmdVer != 'bdl4') {
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

    for (const auto &e : ctx.mVertexBufferMaxIndices) {
      switch (e.first) {
      case gx::VertexBufferAttribute::Position:
        if (ctx.mdl.get().mBufs.pos.mData.size() != e.second + 1) {
          printf("The position vertex buffer currently has %u greedily-claimed "
                 "entries due to 32B padding; %u are used.\n",
                 (u32)ctx.mdl.get().mBufs.pos.mData.size(), e.second + 1);
          ctx.mdl.get().mBufs.pos.mData.resize(e.second + 1);
        }
        break;
      case gx::VertexBufferAttribute::Color0:
      case gx::VertexBufferAttribute::Color1: {
        auto &buf =
            ctx.mdl.get().mBufs.color[(int)e.first -
                                      (int)gx::VertexBufferAttribute::Color0];
        if (buf.mData.size() != e.second + 1) {
          printf("The color buffer currently has %u greedily-claimed entries "
                 "due to 32B padding; %u are used.\n",
                 (u32)buf.mData.size(), e.second + 1);
          buf.mData.resize(e.second + 1);
        }
        break;
      }
      case gx::VertexBufferAttribute::Normal: {
        auto &buf = ctx.mdl.get().mBufs.norm;
        if (buf.mData.size() != e.second + 1) {
          printf("The normal buffer currently has %u greedily-claimed entries "
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
        auto &buf =
            ctx.mdl.get().mBufs.uv[(int)e.first -
                                   (int)gx::VertexBufferAttribute::TexCoord0];
        if (buf.mData.size() != e.second + 1) {
          printf("The UV buffer currently has %u greedily-claimed entries due "
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

    // Read TEX1
    readTEX1(ctx);

    // Read INF1
    readINF1(ctx);

    // Read MDL3
  }
};
void InstallBMD(kpi::ApplicationPlugins &installer) {
  installer.addDeserializer<BMD>();
  installer.addSerializer<BMD>();
}

} // namespace riistudio::j3d
