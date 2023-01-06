#include "../OutputCtx.hpp"
#include "../SceneGraph.hpp"
#include "../Sections.hpp"

namespace librii::j3d {

struct BinaryINF1Header {
  u16 flag{};
  u16 _02{0xFFFF};
  u32 nPacket{};
  u32 nVertex{};

  static Result<BinaryINF1Header> read(rsl::SafeReader& reader) {
    BinaryINF1Header tmp;
    tmp.flag = TRY(reader.U16());
    tmp._02 = TRY(reader.U16());
    tmp.nPacket = TRY(reader.U32());
    tmp.nVertex = TRY(reader.U32());
    return tmp;
  }
  void write(oishii::Writer& writer) {
    writer.write<u16>(flag);
    writer.write<u16>(_02);
    writer.write<u32>(nPacket);
    writer.write<u32>(nVertex);
  }
};

Result<void> readINF1(BMDOutputContext& ctx) {
  rsl::SafeReader reader(ctx.reader);
  if (enterSection(ctx, 'INF1')) {
    ScopedSection g(reader.getUnsafe(), "Information");

    auto header = TRY(BinaryINF1Header::read(reader));
    s32 ofsScnGraph = TRY(reader.S32());

    // TODO: Validate _02, nPacket, nVertex
    ctx.mdl.scalingRule = TRY(rsl::enum_cast<ScalingRule>(header.flag & 0xf));
    EXPECT((header.flag & (~0xf)) == 0);
    reader.seekSet(g.start + ofsScnGraph);
    TRY(SceneGraph::onRead(reader, ctx));
  }

  return {};
}

struct INF1Node {
  static const char* getNameId() { return "INF1 InFormation"; }
  virtual const oishii::Node& getSelf() const = 0;

  void write(oishii::Writer& writer) const {
    if (!mdl)
      return;

    writer.write<u32, oishii::EndianSelect::Big>('INF1');
    writer.writeLink<s32>(oishii::Link{
        oishii::Hook(getSelf()),
        oishii::Hook("VTX1" /*getSelf(), oishii::Hook::EndOfChildren*/)});

    u32 num_mprim = 0;
    for (auto& shp : mdl->shapes)
      num_mprim += shp.mMatrixPrimitives.size();

    BinaryINF1Header header{
        .flag = static_cast<u16>(static_cast<u16>(mdl->scalingRule) & 0xf),
        ._02 = 0xFFFF,
        .nPacket = num_mprim,
        .nVertex = static_cast<u32>(mdl->vertexData.pos.mData.size()),
    };
    header.write(writer);

    writer.writeLink<s32>(
        oishii::Link{oishii::Hook(getSelf()), oishii::Hook("SceneGraph")});
  }

  void gatherChildren(oishii::Node::NodeDelegate& out) const {
    out.addNode(SceneGraph::getLinkerNode(*mdl));
  }
  INF1Node(BMDExportContext& ctx) : mdl(&ctx.mdl) {}
  J3dModel* mdl{nullptr};
};
std::unique_ptr<oishii::Node> makeINF1Node(BMDExportContext& ctx) {
  return std::make_unique<LinkNode<INF1Node>>(ctx);
}

} // namespace librii::j3d
