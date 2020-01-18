#pragma once

#include "SceneGraph.hpp"
#include <oishii/writer/node.hxx>
namespace libcube::jsystem {

enum class ByteCodeOp : s16
{
	Terminate,
	Open,
	Close,

	Joint = 0x10,
	Material,
	Shape,

	Unitialized = -1
};

static_assert(sizeof(ByteCodeOp) == 2, "Invalid enum size.");

struct ByteCodeCmd
{
	ByteCodeOp op;
	s16 idx;

	ByteCodeCmd(oishii::BinaryReader& reader)
	{
		transfer(reader);
	}
	ByteCodeCmd()
		: op(ByteCodeOp::Unitialized), idx(-1)
	{}

	ByteCodeCmd(ByteCodeOp o, s16 i = 0)
		: op(o), idx(i)
	{}
	template<typename T>
	void transfer(T& stream)
	{
		stream.transfer<ByteCodeOp>(op);
		stream.transfer<s16>(idx);

		//	DebugReport("Op: %s, Id: %u\n", [](ByteCodeOp o) -> const char* {
		//		switch (o) {
		//		case ByteCodeOp::Terminate:
		//			return "Terminate";
		//		case ByteCodeOp::Open:
		//			return "Open";
		//		case ByteCodeOp::Close:
		//			return "Close";
		//		case ByteCodeOp::Joint:
		//			return "Joint";
		//		case ByteCodeOp::Material:
		//			return "Material";
		//		case ByteCodeOp::Shape:
		//			return "Shape";
		//		default:
		//			return "?";
		//		}}(op), (u32)idx);
	}
};

void SceneGraph::onRead(oishii::BinaryReader& reader, BMDOutputContext& ctx)
{
	// FIXME: Algorithm can be significantly improved

	u16 mat, joint = 0;
	auto lastType = ByteCodeOp::Unitialized;

	std::vector<ByteCodeOp> hierarchy_stack;
	std::vector<u16> joint_stack;

	for (ByteCodeCmd cmd(reader); cmd.op != ByteCodeOp::Terminate; cmd.transfer(reader))
	{
		switch (cmd.op)
		{
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

			if (!joint_stack.empty())
			{
				ctx.mdl.mJoints[ctx.jointIdLut[joint_stack.back()]].children.emplace_back(ctx.mdl.mJoints[ctx.jointIdLut[newId]].id);
				ctx.mdl.mJoints[ctx.jointIdLut[newId]].parent = ctx.mdl.mJoints[ctx.jointIdLut[joint_stack.back()]].id;
			}
			joint = newId;
			break;
		}
		case ByteCodeOp::Material:
			mat = cmd.idx;
			break;
		case ByteCodeOp::Shape:
			if (mat < ctx.mdl.mMaterials.size()) ctx.mdl.mJoints[ctx.jointIdLut[joint]].displays.emplace_back(ctx.mdl.mMaterials[mat].id, cmd.idx);
			break;
		}

		if (cmd.op != ByteCodeOp::Open && cmd.op != ByteCodeOp::Close)
			lastType = cmd.op;
	}
}
struct SceneGraphNode : public oishii::Node
{
	SceneGraphNode(const J3DModel& mdl, bool linkerOwned = true)
		: Node("SceneGraph"), mdl(mdl)
	{
		transferOwnershipToLinker(linkerOwned);
		getLinkingRestriction().options |= oishii::LinkingRestriction::Leaf;
	}

	eResult write(oishii::Writer& writer) const noexcept
	{
		u32 depth = 0;

		writeBone(writer, mdl.mJoints[0], mdl, depth); // Assume root 0

		ByteCodeCmd(ByteCodeOp::Terminate).transfer(writer);
		return eResult::Success;
	}
	
	void writeBone(oishii::Writer& writer, const Joint& joint, const J3DModel& mdl, u32& depth) const
	{
		u32 startDepth = depth;

		s16 id = -1;
		for (int i = 0; i < mdl.mJoints.size(); ++i)
		{
			if (mdl.mJoints[i].id == joint.id)
				id = i;
		}
		assert(id != -1);
		ByteCodeCmd(ByteCodeOp::Joint, id).transfer(writer);
		ByteCodeCmd(ByteCodeOp::Open).transfer(writer);
		++depth;

		if (!joint.displays.empty())
		{
			Joint::Display last = { -1, -1 };
			for (const auto& d : joint.displays)
			{
				if (d.material != last.material)
				{
					s16 mid = -1;
					for (int i = 0; i < mdl.mMaterials.size(); ++i)
					{
						if (mdl.mMaterials[i].id == d.material)
							mid = i;
					}
					assert(mid != -1);

					ByteCodeCmd(ByteCodeOp::Material, mid).transfer(writer);
					ByteCodeCmd(ByteCodeOp::Open).transfer(writer);
					++depth;
				}
				if (d.shape != last.shape)
				{
					// TODO
					ByteCodeCmd(ByteCodeOp::Shape, d.shape).transfer(writer);
					ByteCodeCmd(ByteCodeOp::Open).transfer(writer);
					++depth;
				}
				last = d;
			}
		}


		for (const auto d : joint.children)
		{
			writeBone(writer, mdl.mJoints[d], mdl, depth);
		}
		if (depth != startDepth)
		{
			// If last is an open, overwrite it
			if (oishii::swapEndian(*(u16*)(writer.getDataBlockStart() + writer.tell() - 4)) == 1)
			{
				writer.seek(-4);
				--depth;
			}
			for (u32 i = startDepth; i < depth; ++i)
			{
				ByteCodeCmd(ByteCodeOp::Close).transfer(writer);
			}
			depth = startDepth;
		}
	}

	const J3DModel& mdl;
};
oishii::Node* SceneGraph::getLinkerNode(const J3DModel& mdl, bool linkerOwned)
{
	return new SceneGraphNode(mdl, linkerOwned);
}
} // namespace libcube::jsystem
