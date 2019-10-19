#include "ArcDir.hpp"

void Dir::onRead(oishii::BinaryReader& bReader, Dir& context)
{
	context.m_sizeOfFile = bReader.read<u32>();
	context.m_entries.resize(bReader.read<u32>());

	for (auto& entry : context.m_entries)
	{
		entry.m_offset = bReader.read<u32>();
		entry.m_size = bReader.read<u32>();
		entry.m_path.resize(bReader.read<u32>());
		for (u32 i = 0; i < entry.m_path.size(); i++)
			entry.m_path[i] = bReader.read<u8>();

		// TODO, opening and reading arc file
	}
}