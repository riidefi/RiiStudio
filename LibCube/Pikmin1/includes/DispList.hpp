#pragma once

#include "essential_functions.hpp"

namespace libcube { namespace pikmin1 {

struct DispList
{
	constexpr static const char name[] = "Display List";

	u32 m_unk1 = 0;
	u32 m_unk2 = 0;

	std::vector<u8> m_dispData;

	DispList() = default;
	~DispList() = default;

	static void onRead(oishii::BinaryReader& bReader, DispList& context)
	{
		context.m_unk1 = bReader.readUnaligned<u32>();
		context.m_unk2 = bReader.readUnaligned<u32>();
		context.m_dispData.resize(bReader.readUnaligned<u32>());

		skipPadding(bReader);
		for (auto& dispData : context.m_dispData)
			dispData = bReader.readUnaligned<u8>();
	}
};

inline void operator<<(DispList& context, oishii::BinaryReader& bReader)
{
	bReader.dispatch<DispList, oishii::Direct, false>(context);
}


}

}
