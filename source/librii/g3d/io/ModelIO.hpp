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

enum class RenderCommand {
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
  struct Return {};
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
  using Command = std::variant<NoOp, Return, Draw, NodeDescendence>;

  static std::vector<Command> ParseStream(oishii::BinaryReader& reader) {
    std::vector<Command> commands;
    while (reader.tell() < reader.endpos()) {
      const auto cmd = static_cast<RenderCommand>(reader.read<u8>());
      switch (cmd) {
      case RenderCommand::NoOp:
        break;
      default:
        assert(!"Unexpected bytecode token");
        return commands;
      case RenderCommand::Return:
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
