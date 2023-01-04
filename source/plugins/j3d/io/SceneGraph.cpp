#include "SceneGraph.hpp"
#include <oishii/writer/node.hxx>

namespace riistudio::j3d {

enum class ByteCodeOp : s16 {
  Terminate,
  Open,
  Close,

  Joint = 0x10,
  Material,
  Shape,

  Unitialized = -1
};

static_assert(sizeof(ByteCodeOp) == 2, "Invalid enum size.");

struct ByteCodeCmd {
  ByteCodeOp op;
  s16 idx;

  ByteCodeCmd(oishii::BinaryReader& reader) { transfer(reader); }
  ByteCodeCmd() : op(ByteCodeOp::Unitialized), idx(-1) {}

  ByteCodeCmd(ByteCodeOp o, s16 i = 0) : op(o), idx(i) {}
  template <typename T> void transfer(T& stream) {
    stream.template transfer<ByteCodeOp>(op);
    stream.template transfer<s16>(idx);

#if 0
    DebugReport(
        "Op: %s, Id: %u\n",
        [](ByteCodeOp o) -> const char* {
          switch (o) {
          case ByteCodeOp::Terminate:
            return "Terminate";
          case ByteCodeOp::Open:
            return "Open";
          case ByteCodeOp::Close:
            return "Close";
          case ByteCodeOp::Joint:
            return "Joint";
          case ByteCodeOp::Material:
            return "Material";
          case ByteCodeOp::Shape:
            return "Shape";
          default:
            return "?";
          }
        }(op),
        (u32)idx);
#endif
  }
};

void SceneGraph::onRead(oishii::BinaryReader& reader, BMDOutputContext& ctx) {
  // FIXME: Algorithm can be significantly improved
  u16 mat = 0, joint = 0;
  auto lastType = ByteCodeOp::Unitialized;

  std::vector<ByteCodeOp> hierarchy_stack;
  std::vector<u16> joint_stack;

  auto& joints = ctx.mdl.joints;

  for (ByteCodeCmd cmd(reader); cmd.op != ByteCodeOp::Terminate;
       cmd.transfer(reader)) {
    switch (cmd.op) {
    case ByteCodeOp::Terminate:
      return;
    case ByteCodeOp::Open:
      if (lastType == ByteCodeOp::Joint)
        joint_stack.push_back(joint);
      hierarchy_stack.emplace_back(lastType);
      break;
    case ByteCodeOp::Close:
      if (hierarchy_stack.back() == ByteCodeOp::Joint)
        joint_stack.pop_back();
      hierarchy_stack.pop_back();
      break;
    case ByteCodeOp::Joint: {
      const auto newId = cmd.idx;

      if (!joint_stack.empty()) {
        joints[joint_stack.back()].children.emplace_back(
            ctx.mdl.jointIds[newId]);
        joints[newId].parentId = ctx.mdl.jointIds[joint_stack.back()];
      }
      joint = newId;
      break;
    }
    case ByteCodeOp::Material:
      mat = cmd.idx;
      break;
    case ByteCodeOp::Shape: {
      assert(mat < ctx.mdl.materials.size());
      auto& displays = joints[joint].displays;
      displays.emplace_back(mat, cmd.idx);
    } break;
    case ByteCodeOp::Unitialized:
    default:
      assert(!"Invalid bytecode op");
      break;
    }

    if (cmd.op != ByteCodeOp::Open && cmd.op != ByteCodeOp::Close)
      lastType = cmd.op;
  }
}
struct SceneGraphNode : public oishii::Node {
  SceneGraphNode(const J3dModel& mdl) : Node("SceneGraph"), mdl(mdl) {
    getLinkingRestriction().setLeaf();
  }

  Result write(oishii::Writer& writer) const noexcept {
    u32 depth = 0;

    // Assume root 0
    writeBone(writer, mdl.joints[0], mdl.jointIds[0], mdl, depth);

    ByteCodeCmd(ByteCodeOp::Terminate).transfer(writer);
    return eResult::Success;
  }

  void writeBone(oishii::Writer& writer, const librii::j3d::JointData& joint,
                 u32 jointId, const J3dModel& mdl, u32& depth) const {
    u32 startDepth = depth;

    s16 id = -1;
    for (int i = 0; i < mdl.joints.size(); ++i) {
      if (mdl.jointIds[i] == jointId)
        id = i;
    }
    assert(id != -1);
    ByteCodeCmd(ByteCodeOp::Joint, id).transfer(writer);
    ByteCodeCmd(ByteCodeOp::Open).transfer(writer);
    ++depth;

    if (!joint.displays.empty()) {
      librii::j3d::JointData::Display last = {-1, -1};
      for (const auto& d : joint.displays) {
        if (d.material != last.material) {
          // s16 mid = -1;
          // for (int i = 0; i < mdl.getMaterials().size(); ++i) {
          //   if (mdl.getMaterial(i).get().id == d.material)
          //     mid = i;
          // }
          // assert(mid != -1);

          // TODO: Ensure index=id is true before calling this
          s16 mid = d.material;

          ByteCodeCmd(ByteCodeOp::Material, mid).transfer(writer);
          ByteCodeCmd(ByteCodeOp::Open).transfer(writer);
          ++depth;
        }
        if (d.shape != last.shape) {
          // TODO
          ByteCodeCmd(ByteCodeOp::Shape, d.shape).transfer(writer);
          ByteCodeCmd(ByteCodeOp::Open).transfer(writer);
          ++depth;
        }
        last = d;
      }
    }

    for (const auto d : joint.children) {
      writeBone(writer, mdl.joints[d], mdl.jointIds[d], mdl, depth);
    }
    if (depth != startDepth) {
      // If last is an open, overwrite it
      if (oishii::swapEndian(
              *(u16*)(writer.getDataBlockStart() + writer.tell() - 4)) == 1) {
        writer.skip(-4);
        --depth;
      }
      for (u32 i = startDepth; i < depth; ++i) {
        ByteCodeCmd(ByteCodeOp::Close).transfer(writer);
      }
      depth = startDepth;
    }
  }

  const J3dModel& mdl;
};
std::unique_ptr<oishii::Node> SceneGraph::getLinkerNode(const J3dModel& mdl) {
  return std::make_unique<SceneGraphNode>(mdl);
}
} // namespace riistudio::j3d
