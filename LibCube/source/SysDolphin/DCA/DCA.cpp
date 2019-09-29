#include "DCA.hpp"

namespace libcube { namespace pikmin1 {

void libcube::pikmin1::DCA::onRead(oishii::BinaryReader& bReader, DCA& context)
{
	context.m_numJoints = bReader.read<u32>();
	context.m_numFrames = bReader.read<u32>();
	read(bReader, context.m_scaling);
	read(bReader, context.m_rotation);
	read(bReader, context.m_translation);

	context.m_joints.resize(context.m_numJoints);
	for (auto& joint : context.m_joints)
	{
		joint.index = bReader.read<u32>();
		joint.parent = bReader.read<u32>();
		// Does some reassigning if joint.parent == -1, but decompiling that class is wayy too hard
		for (u32 i = 0; i < 3; i++)
		{
			joint.sx_param[i].first = bReader.read<u32>();
			joint.sy_param[i].second = bReader.read<u32>();
		}
		for (u32 i = 0; i < 3; i++)
		{
			joint.rx_param[i].first = bReader.read<u32>();
			joint.ry_param[i].second = bReader.read<u32>();
		}
		for (u32 i = 0; i < 3; i++)
		{
			joint.tx_param[i].first = bReader.read<u32>();
			joint.ty_param[i].second = bReader.read<u32>();
		}
	}
}

}

}
