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
  // TODO: NodeMix
  struct Draw {
    u16 matId;
    u16 polyId;
    u16 boneId;
    u8 prio;
  };
  // TODO: EnvelopeMatrix
  // TODO: MatrixDuplicate
  using Command = std::variant<NoOp, Draw, NodeDescendence>;

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
};

class BinaryModel {
public:
  librii::g3d::G3DModelDataData info = {};
  std::vector<librii::g3d::BoneData> bones;

  std::vector<librii::g3d::PositionBuffer> positions;
  std::vector<librii::g3d::NormalBuffer> normals;
  std::vector<librii::g3d::ColorBuffer> colors;
  std::vector<librii::g3d::TextureCoordinateBuffer> texcoords;

  std::vector<librii::g3d::G3dMaterialData> materials;
  std::vector<librii::g3d::PolygonData> meshes;

  // This has yet to be applied to the bones/draw matrices
  std::vector<ByteCodeMethod> bytecodes;

  void read(oishii::BinaryReader& reader, kpi::LightIOTransaction& transaction,
            const std::string& transaction_path, bool& isValid);
};

} // namespace librii::g3d
