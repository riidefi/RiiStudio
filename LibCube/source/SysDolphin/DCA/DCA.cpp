#include "DCA.hpp"

namespace libcube { namespace pikmin1 {

void DCA::onRead(oishii::BinaryReader& bReader, DCA& context)
{
	context.m_numJoints = bReader.read<u32>();
	context.m_numFrames = bReader.read<u32>();
	context.m_scaling << bReader;
	context.m_rotation << bReader;
	context.m_translation << bReader;

	context.m_joints.resize(context.m_numJoints);
	for (auto& joint : context.m_joints)
	{
		joint.index = bReader.read<u32>();
		joint.parent = bReader.read<u32>();
		if (joint.parent == -1)
			DebugReport("Joint %d has no parent!", joint.index);
		for (u32 i = 0; i < 3; i++)
		{
			joint.sx_param[i] = bReader.read<u32>();
			joint.sy_param[i] = bReader.read<u32>();
		}
		for (u32 i = 0; i < 3; i++)
		{
			joint.rx_param[i] = bReader.read<u32>();
			joint.ry_param[i] = bReader.read<u32>();
		}
		for (u32 i = 0; i < 3; i++)
		{
			joint.tx_param[i] = bReader.read<u32>();
			joint.ty_param[i] = bReader.read<u32>();
		}
	}
}

}

}
