#include <LibJ/J3D/Binary/BMD/Sections.hpp>
#include "../SceneGraph.hpp"
namespace libcube::jsystem {

void readINF1(BMDOutputContext& ctx)
{
    auto& reader = ctx.reader;
	if (enterSection(ctx, 'INF1'))
	{
		ScopedSection g(reader, "Information");

		u16 flag = reader.read<u16>();
		reader.signalInvalidityLast<u16, oishii::UncommonInvalidity>("Flag");
		reader.read<u16>();

		// TODO -- Use these for validation
		// u32 nPacket =
			reader.read<u32>();
		// u32 nVertex =
			reader.read<u32>();

		ctx.mdl.info.mScalingRule = static_cast<J3DModel::Information::ScalingRule>(flag & 0xf);
		// FIXME
		//assert((flag & ~0xf) == 0);

		reader.dispatch<SceneGraph, oishii::Indirection<0, s32, oishii::Whence::At>>(ctx, g.start);
	}
}

struct INF1Node
{
	static const char* getNameId() { return "INF1 InFormation"; }
	virtual const oishii::v2::Node& getSelf() const = 0;

	void write(oishii::v2::Writer& writer) const
	{
		if (!mdl) return;

		writer.write<u32, oishii::EndianSelect::Big>('INF1');
		writer.writeLink<s32>(oishii::v2::Link{
			oishii::v2::Hook(getSelf()),
			oishii::v2::Hook("VTX1"/*getSelf(), oishii::Hook::EndOfChildren*/) });

		writer.write<u16>(static_cast<u16>(mdl->info.mScalingRule) & 0xf);
		writer.write<u16>(-1);
		// Matrix primitive count
		u64 num_mprim = 0;
		for (const auto& it : mdl->mShapes)
			num_mprim += it.getNumMatrixPrimitives();
		writer.write<u32>(num_mprim);
		// Vertex position count
		writer.write<u32>((u32)mdl->mBufs.pos.mData.size());

		writer.writeLink<s32>(oishii::v2::Link{
			oishii::v2::Hook(getSelf()),
			oishii::v2::Hook("SceneGraph")
			});
	}


	void gatherChildren(oishii::v2::Node::NodeDelegate& out) const
	{
		out.addNode(SceneGraph::getLinkerNode(*mdl));
	}
	INF1Node(BMDExportContext& ctx) : mdl(&ctx.mdl)
	{}
	J3DModel* mdl = nullptr;
};
std::unique_ptr<oishii::v2::Node> makeINF1Node(BMDExportContext& ctx)
{
	return std::make_unique<LinkNode<INF1Node>>(ctx);
}

}
