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

  static Result<std::vector<Command>>
  ParseStream(oishii::BinaryReader& unsafeReader, bool keep_nops = false) {
    rsl::SafeReader reader(unsafeReader);
    std::vector<Command> commands;
    while (reader.tell() < unsafeReader.endpos()) {
      const auto cmd = TRY(reader.Enum8<RenderCommand>());
      switch (cmd) {
      case RenderCommand::NoOp:
        if (keep_nops) {
          commands.push_back(NoOp{});
        }
        continue;
      case RenderCommand::Return:
        // NOTE: Not captured in stream
        return commands;
      case RenderCommand::Draw: {
        Draw draw;
        draw.matId = TRY(reader.U16NoAlign());
        draw.polyId = TRY(reader.U16NoAlign());
        draw.boneId = TRY(reader.U16NoAlign());
        draw.prio = TRY(reader.U8NoAlign());
        commands.push_back(draw);
        continue;
      }
      case RenderCommand::NodeDescendence: {
        NodeDescendence desc;
        desc.boneId = TRY(reader.U16NoAlign());
        desc.parentMtxId = TRY(reader.U16NoAlign());
        commands.push_back(desc);
        continue;
      }
      case RenderCommand::EnvelopeMatrix: {
        EnvelopeMatrix evp;
        evp.mtxId = TRY(reader.U16NoAlign());
        evp.nodeId = TRY(reader.U16NoAlign());
        commands.push_back(evp);
        continue;
      }
      case RenderCommand::NodeMixing: {
        NodeMix mix;
        mix.mtxId = TRY(reader.U16NoAlign());
        auto numBlend = TRY(reader.U8NoAlign());
        mix.blendMatrices.resize(numBlend);
        for (u8 i = 0; i < numBlend; ++i) {
          mix.blendMatrices[i].mtxId = TRY(reader.U16NoAlign());
          mix.blendMatrices[i].ratio = TRY(reader.F32NoAlign());
        }
        commands.push_back(mix);
        continue;
      }
      }
      EXPECT(false, "Unexpected bytecode token in stream. (We shouldn't even "
                    "reach here as Enum8<RenderCommand> should validate cmd.)");
    }
    EXPECT(false, "End of file");
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
    Result<void> readAt(oishii::BinaryReader& reader, u32 pos) {
      auto count = TRY(reader.tryGetAt<u32>(pos));
      if (pos + count * 4 >= reader.endpos()) {
        return std::unexpected("mtxIdToBoneId LUT size exceeds filesize");
      }
      mtxIdToBoneId.resize(count);
      for (u32 i = 0; i < count; ++i) {
        auto entry = TRY(reader.tryGetAt<s32>(pos + 4 + i * 4));
        mtxIdToBoneId[i] = entry;
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

  Result<void> read(rsl::SafeReader& reader) {
    auto infoPos = reader.tell();
    // Ignore size
    TRY(reader.U32());
    // Ignore ofsModel
    TRY(reader.S32());
    scalingRule = TRY(reader.Enum32<librii::g3d::ScalingRule>());
    texMtxMode = TRY(reader.Enum32<librii::g3d::TextureMatrixMode>());
    numVerts = TRY(reader.U32());
    numTris = TRY(reader.U32());
    sourceLocation = TRY(reader.StringOfs(infoPos));
    numViewMtx = TRY(reader.U32());
    normalMtxArray = TRY(reader.U8());
    texMtxArray = TRY(reader.U8());
    boundVolume = TRY(reader.U8());
    evpMtxMode = TRY(reader.Enum8<librii::g3d::EnvelopeMatrixMode>());
    // This is not customizable
    u32 ofsMtxToBoneLUT = TRY(reader.S32());
    min.x = TRY(reader.F32());
    min.y = TRY(reader.F32());
    min.z = TRY(reader.F32());
    max.x = TRY(reader.F32());
    max.y = TRY(reader.F32());
    max.z = TRY(reader.F32());
    TRY(mtxToBoneLUT.readAt(reader.getUnsafe(), infoPos + ofsMtxToBoneLUT));
    return {};
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

  Result<void> read(oishii::BinaryReader& reader,
                    kpi::LightIOTransaction& transaction,
                    const std::string& transaction_path, bool& isValid);
  void write(oishii::Writer& writer, NameTable& names, std::size_t brres_start);
};

} // namespace librii::g3d
