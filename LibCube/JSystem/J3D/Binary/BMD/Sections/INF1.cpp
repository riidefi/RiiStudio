#include <LibCube/JSystem/J3D/Binary/BMD/Sections.hpp>
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
		u32 nPacket = reader.read<u32>();
		u32 nVertex = reader.read<u32>();

		ctx.mdl.info.mScalingRule = static_cast<J3DModel::Information::ScalingRule>(flag & 0xf);
		// FIXME
		//assert((flag & ~0xf) == 0);

		reader.dispatch<SceneGraph, oishii::Indirection<0, s32, oishii::Whence::At>>(ctx, g.start);
	}
}

}
