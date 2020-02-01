#pragma once

#include "essential_functions.hpp"

namespace libcube { namespace pikmin1 {

struct TexAttr
{
	constexpr static const char name[] = "Texture Attribute";

	enum class Mode : u16
	{
		Default = 0,
		Unknown1,
		Edge, //!< EDG
		Unknown2,
		Translucent //!< XLU
	};

	// If m_image is -1, its not using an image,
	// If m_image isn't -1, its the index of the image TexAttr is going to attach to (unsure)
	u16 m_image = -1;
	// context: plugTexConv assigns to this when "tiling" and then "clamp" is found
	u16 m_tilingMode;
	u16 m_unk1;
	u16 m_unk2;
	f32 m_unk3;
	char padding[2];

	TexAttr() = default;
	~TexAttr() = default;

	static void onRead(oishii::BinaryReader& bReader, TexAttr& context)
	{
		context.m_image = bReader.read<u32>();
		context.m_tilingMode = bReader.read<u16>();
		context.m_unk2 = bReader.read<u16>();
		context.m_unk3 = bReader.read<f32>();
	}
};

inline void operator<<(TexAttr& evp, oishii::BinaryReader& reader)
{
	reader.dispatch<TexAttr, oishii::Direct, false>(evp);
}

} }
