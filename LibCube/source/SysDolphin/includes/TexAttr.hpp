#pragma once

#include "essential_functions.hpp"

namespace libcube { namespace pikmin1 {

struct TexAttr
{
	constexpr static const char name[] = "Texture Attribute";

	enum class Mode
	{
		Default = 0,
		Unknown1,
		Edge, //!< EDG
		Unknown2,
		Translucent //!< XLU
	};

	u16 m_imageNum;	//2
	u16 m_tilingMode;	//4
	Mode m_mode;	//6
	u16 m_unk1;
	f32 m_unk2;	//10

	char padding[2];	//12

	static void onRead(oishii::BinaryReader& bReader, TexAttr& context)
	{
		context.m_imageNum = bReader.read<u16>();
		context.m_tilingMode = bReader.read<u16>();
		context.m_mode = static_cast<Mode>(bReader.read<u16>());
		bReader.read<u16>();
		context.m_unk2 = bReader.read<f32>();
	}
};

inline void read(oishii::BinaryReader& reader, TexAttr& attr)
{
	reader.dispatch<TexAttr, oishii::Direct, false>(attr);
}

} }
