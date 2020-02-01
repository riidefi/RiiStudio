#pragma once

#include "essential_functions.hpp"
#include <string>

namespace libcube { namespace pikmin1 {

struct String {
	constexpr static const char name[] = "Pikmin 1 String";

	std::string m_str;

	static void onRead(oishii::BinaryReader& bReader, String& context)
	{
		const u32 nameLength = bReader.read<u32>();
		std::string nameString(nameLength, 0);

		for (u32 j = 0; j < nameLength; ++j)
			nameString[j] = bReader.read<s8>();

		context.m_str = nameString;
	}
};
inline void operator<<(String& context, oishii::BinaryReader& bReader)
{
	bReader.dispatch<String, oishii::Direct, false>(context);
}


}

}
