#include "../../Scene.hpp"
#include "../OutputCtx.hpp"
#include "../SceneGraph.hpp"
#include "../Sections.hpp"
namespace riistudio::j3d {

void readINF1(BMDOutputContext& ctx) {
  auto& reader = ctx.reader;
  if (enterSection(ctx, 'INF1')) {
    ScopedSection g(reader, "Information");

    u16 flag = reader.read<u16>();
    // reader.signalInvalidityLast<u16, oishii::UncommonInvalidity>("Flag");
    reader.read<u16>();

    // TODO -- Use these for validation
    // u32 nPacket =
    reader.read<u32>();
    // u32 nVertex =
    reader.read<u32>();

    ctx.mdl.info.mScalingRule =
        static_cast<Model::Information::ScalingRule>(flag & 0xf);
    // FIXME
    // assert((flag & ~0xf) == 0);
    reader.seekSet(g.start + reader.read<s32>());
    SceneGraph::onRead(reader, ctx);
  }
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

    writer.write<u16>(static_cast<u16>(mdl->info.mScalingRule) & 0xf);
    writer.write<u16>(-1);
    // Matrix primitive count
    u64 num_mprim = 0;
    for (auto& shp : mdl->getMeshes())
      num_mprim += shp.getMeshData().mMatrixPrimitives.size();
    writer.write<u32>(num_mprim);
    // Vertex position count
    writer.write<u32>((u32)mdl->mBufs.pos.mData.size());

    writer.writeLink<s32>(
        oishii::Link{oishii::Hook(getSelf()), oishii::Hook("SceneGraph")});
  }

  void gatherChildren(oishii::Node::NodeDelegate& out) const {
    out.addNode(SceneGraph::getLinkerNode(*mdl));
  }
  INF1Node(BMDExportContext& ctx) : mdl(&ctx.mdl) {}
  Model* mdl{nullptr};
};
std::unique_ptr<oishii::Node> makeINF1Node(BMDExportContext& ctx) {
  return std::make_unique<LinkNode<INF1Node>>(ctx);
}

} // namespace riistudio::j3d
