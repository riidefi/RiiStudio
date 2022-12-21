#pragma once

#include <core/common.h>
#include <librii/g3d/data/BoneData.hpp>
#include <librii/g3d/data/MaterialData.hpp>
#include <librii/g3d/data/ModelData.hpp>
#include <librii/g3d/data/PolygonData.hpp>
#include <librii/g3d/data/TextureData.hpp>
#include <librii/g3d/data/VertexData.hpp>
#include <oishii/reader/binary_reader.hxx>
#include <variant>
#include <vector>

// Regrettably, for kpi::LightIOTransaction
#include <core/kpi/Plugins.hpp>

// readName
#include <librii/g3d/io/CommonIO.hpp>
// NameTable
#include <librii/g3d/io/NameTableIO.hpp>

// BinaryMaterial
#include <librii/g3d/io/MatIO.hpp>

namespace librii::g3d {

enum class RenderCommand : u8 {
  NoOp,
  Return,

  NodeDescendence,
  NodeMixing,

  Draw,

  EnvelopeMatrix,
  MatrixCopy
};

struct ByteCodeLists {
  struct NoOp {};
  struct NodeDescendence {
    u16 boneId;
    u16 parentMtxId;
  };
  struct NodeMix {
    u16 mtxId;
    struct BlendMtx {
      u16 mtxId;
      f32 ratio;
    };
    std::vector<BlendMtx> blendMatrices;
  };
  struct Draw {
    u16 matId;
    u16 polyId;
    u16 boneId;
    u8 prio;
  };
  struct EnvelopeMatrix {
    u16 mtxId;
    u16 nodeId;
  };
  // TODO: MatrixDuplicate
  using Command =
      std::variant<NoOp, Draw, NodeDescendence, EnvelopeMatrix, NodeMix>;

  static std::vector<Command> ParseStream(oishii::BinaryReader& reader,
                                          bool keep_nops = false) {
    std::vector<Command> commands;
    while (reader.tell() < reader.endpos()) {
      const auto cmd = static_cast<RenderCommand>(reader.read<u8>());
      switch (cmd) {
      case RenderCommand::NoOp:
        if (keep_nops) {
          commands.push_back(NoOp{});
        }
        break;
      default:
        assert(!"Unexpected bytecode token");
        return commands;
      case RenderCommand::Return:
        // NOTE: Not captured in stream
        return commands;
      case RenderCommand::Draw: {
        Draw draw;
        draw.matId = reader.readUnaligned<u16>();
        draw.polyId = reader.readUnaligned<u16>();
        draw.boneId = reader.readUnaligned<u16>();
        draw.prio = reader.readUnaligned<u8>();
        commands.push_back(draw);
        break;
      }
      case RenderCommand::NodeDescendence: {
        NodeDescendence desc;
        desc.boneId = reader.readUnaligned<u16>();
        desc.parentMtxId = reader.readUnaligned<u16>();
        commands.push_back(desc);
        break;
      }
      case RenderCommand::EnvelopeMatrix: {
        EnvelopeMatrix evp;
        evp.mtxId = reader.readUnaligned<u16>();
        evp.nodeId = reader.readUnaligned<u16>();
        commands.push_back(evp);
        break;
      }
      case RenderCommand::NodeMixing: {
        NodeMix mix;
        mix.mtxId = reader.readUnaligned<u16>();
        auto numBlend = reader.readUnaligned<u8>();
        mix.blendMatrices.resize(numBlend);
        for (u8 i = 0; i < numBlend; ++i) {
          mix.blendMatrices[i].mtxId = reader.readUnaligned<u16>();
          mix.blendMatrices[i].ratio = reader.readUnaligned<f32>();
        }
        commands.push_back(mix);
        break;
      }
      }
    }
    assert(!"Reached end of file");
    return {};
  }
  static void WriteStream(oishii::Writer& writer,
                          std::span<const Command> commands) {
    for (const auto& cmd : commands) {
      if (auto* draw = std::get_if<Draw>(&cmd)) {
        writer.writeUnaligned<u8>(static_cast<u8>(RenderCommand::Draw));
        writer.writeUnaligned<u16>(draw->matId);
        writer.writeUnaligned<u16>(draw->polyId);
        writer.writeUnaligned<u16>(draw->boneId);
        writer.writeUnaligned<u8>(draw->prio);
      } else if (auto* draw = std::get_if<NodeDescendence>(&cmd)) {
        writer.writeUnaligned<u8>(
            static_cast<u8>(RenderCommand::NodeDescendence));
        writer.writeUnaligned<u16>(draw->boneId);
        writer.writeUnaligned<u16>(draw->parentMtxId);
      }
      if (auto* evp = std::get_if<EnvelopeMatrix>(&cmd)) {
        writer.writeUnaligned<u8>(
            static_cast<u8>(RenderCommand::EnvelopeMatrix));
        writer.writeUnaligned<u16>(evp->mtxId);
        writer.writeUnaligned<u16>(evp->nodeId);
      } else if (auto* mix = std::get_if<NodeMix>(&cmd)) {
        writer.writeUnaligned<u8>(static_cast<u8>(RenderCommand::NodeMixing));
        writer.writeUnaligned<u16>(mix->mtxId);
        writer.writeUnaligned<u8>(static_cast<u16>(mix->blendMatrices.size()));
        for (const auto& blend : mix->blendMatrices) {
          writer.writeUnaligned<u16>(blend.mtxId);
          writer.writeUnaligned<f32>(blend.ratio);
        }
      } else if (auto* draw = std::get_if<NoOp>(&cmd)) {
        writer.writeUnaligned<u8>(static_cast<u8>(RenderCommand::NoOp));
      } else {
        // TODO: Ignored
      }
    }

    // NOTE: Not captured in stream
    writer.writeUnaligned<u8>(static_cast<u8>(RenderCommand::Return));
  }
};
struct ByteCodeMethod {
  std::string name; // These have special meanings
  std::vector<ByteCodeLists::Command> commands;

  std::string getName() const { return name; }
};

struct BinaryModelInfo {
  librii::g3d::ScalingRule scalingRule = ScalingRule::Maya;
  librii::g3d::TextureMatrixMode texMtxMode = TextureMatrixMode::Maya;
  u32 numVerts = 0;
  u32 numTris = 0;
  std::string sourceLocation;
  u32 numViewMtx = 0;
  bool normalMtxArray = false;
  bool texMtxArray = false;
  bool boundVolume = false;
  librii::g3d::EnvelopeMatrixMode evpMtxMode = EnvelopeMatrixMode::Normal;
  glm::vec3 min{0.0f, 0.0f, 0.0f};
  glm::vec3 max{0.0f, 0.0f, 0.0f};

  struct MtxToBoneLUT {
    std::vector<s32> mtxIdToBoneId;
    std::string readAt(oishii::BinaryReader& reader, u32 pos) {
      auto count = reader.tryGetAt<u32>(pos);
      if (!count) {
        return count.error();
      }
      if (pos + *count * 4 >= reader.endpos()) {
        return "mtxIdToBoneId LUT size exceeds filesize";
      }
      mtxIdToBoneId.resize(*count);
      for (u32 i = 0; i < *count; ++i) {
        auto entry = reader.tryGetAt<s32>(pos + 4 + i * 4);
        if (!entry) {
          return entry.error();
        }
        mtxIdToBoneId[i] = *entry;
      }

      return {};
    }
    void write(oishii::Writer& writer) {
      writer.write<u32>(mtxIdToBoneId.size());
      for (const auto id : mtxIdToBoneId) {
        writer.write<s32>(id);
      }
    }
  };

  MtxToBoneLUT mtxToBoneLUT;

  void read(oishii::BinaryReader& reader) {
    auto infoPos = reader.tell();
    // Ignore size
    reader.read<u32>();
    // Ignore ofsModel
    reader.read<s32>();
    scalingRule = static_cast<librii::g3d::ScalingRule>(reader.read<u32>());
    texMtxMode =
        static_cast<librii::g3d::TextureMatrixMode>(reader.read<u32>());
    numVerts = reader.read<u32>();
    numTris = reader.read<u32>();
    sourceLocation = readName(reader, infoPos);
    numViewMtx = reader.read<u32>();
    normalMtxArray = reader.read<u8>();
    texMtxArray = reader.read<u8>();
    boundVolume = reader.read<u8>();
    evpMtxMode =
        static_cast<librii::g3d::EnvelopeMatrixMode>(reader.read<u8>());
    // This is not customizable
    u32 ofsMtxToBoneLUT = reader.read<s32>();
    min.x = reader.read<f32>();
    min.y = reader.read<f32>();
    min.z = reader.read<f32>();
    max.x = reader.read<f32>();
    max.y = reader.read<f32>();
    max.z = reader.read<f32>();
    mtxToBoneLUT.readAt(reader, infoPos + ofsMtxToBoneLUT);
  }
  void write(oishii::Writer& writer, u32 mdl0Pos) {
    u32 pos = writer.tell();
    u32 size = 0x40;
    u32 ofsMtxToBoneLUT = size;

    writer.write<u32>(size);
    writer.write<s32>(mdl0Pos - pos);
    writer.write<u32>(static_cast<u32>(scalingRule));
    writer.write<u32>(static_cast<u32>(texMtxMode));
    writer.write<u32>(numVerts);
    writer.write<u32>(numTris);
    // TODO: Support sourceLocation
    writer.write<u32>(0);
    writer.write<u32>(numViewMtx);
    writer.write<u8>(normalMtxArray);
    writer.write<u8>(texMtxArray);
    writer.write<u8>(boundVolume);
    writer.write<u8>(static_cast<u8>(evpMtxMode));
    writer.write<u32>(ofsMtxToBoneLUT);
    writer.write<f32>(min.x);
    writer.write<f32>(min.y);
    writer.write<f32>(min.z);
    writer.write<f32>(max.x);
    writer.write<f32>(max.y);
    writer.write<f32>(max.z);
    writer.seekSet(pos + ofsMtxToBoneLUT);
    mtxToBoneLUT.write(writer);
  }
};

class BinaryModel {
public:
  std::string name;
  librii::g3d::BinaryModelInfo info = {};
  std::vector<librii::g3d::BinaryBoneData> bones;

  std::vector<librii::g3d::PositionBuffer> positions;
  std::vector<librii::g3d::NormalBuffer> normals;
  std::vector<librii::g3d::ColorBuffer> colors;
  std::vector<librii::g3d::TextureCoordinateBuffer> texcoords;

  std::vector<librii::g3d::BinaryMaterial> materials;
  // This is the collapsed list, at least for now.
  // That is, tevs[mat.id] isn't valid; you must use tev[mat.tevId] which is
  // generated from mat.tevOffset
  std::vector<librii::g3d::BinaryTev> tevs;

  std::vector<librii::g3d::PolygonData> meshes;

  // This has yet to be applied to the bones/draw matrices
  std::vector<ByteCodeMethod> bytecodes;

  void read(oishii::BinaryReader& reader, kpi::LightIOTransaction& transaction,
            const std::string& transaction_path, bool& isValid);
  void write(oishii::Writer& writer, NameTable& names, std::size_t brres_start,
             // For order of texture name -> TexPlttInfo LUT
             std::span<const librii::g3d::TextureData> textures);
};

} // namespace librii::g3d
