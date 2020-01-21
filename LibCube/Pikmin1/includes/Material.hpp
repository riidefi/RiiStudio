#pragma once

#include "essential_functions.hpp"
#include "Colour.hpp"

namespace libcube { namespace pikmin1 {

// PVWKeyInfoS10 = sizeof(this struct)
struct PVWKeyInfoS10
{
	u16 m_unk1; // 0
	u16 m_unk2; // 2
	f32 m_unk3; // 6
	f32 m_unk4; // 10

	void read(oishii::BinaryReader& bReader)
	{
		m_unk1 = bReader.readUnaligned<u16>();
		m_unk2 = bReader.readUnaligned<u16>();
		m_unk3 = bReader.readUnaligned<f32>();
		m_unk4 = bReader.readUnaligned<f32>();
	}
};

struct PVWKeyInfoU8
{
	// 12 bytes big
	char m_unk1;
	float m_unk2;
	float m_unk3;

	void read(oishii::BinaryReader& bReader)
	{
		m_unk1 = bReader.readUnaligned<u8>();
		bReader.readUnaligned<u8>();
		bReader.readUnaligned<u8>();
		bReader.readUnaligned<u8>();

		m_unk2 = bReader.readUnaligned<f32>();
		m_unk3 = bReader.readUnaligned<f32>();
	}
};

struct PVWKeyInfoF32
{
	float m_keyframeA;
	float m_keyframeB;
	float m_keyframeC;
};


// USED FOR THE PVWTEVINFO STRUCT
struct PVWCombiner
{
	u8 m_unk1;
	u8 m_unk2;
	u8 m_unk3;
	u8 m_unk4;
	u8 m_unk5;
	u8 m_unk6;
	u8 m_unk7;
	u8 m_unk8;
	u8 m_unk9;
	u8 m_unk10;
	u8 m_unk11;
	u8 m_unk12;

	void read(oishii::BinaryReader& bReader)
	{
		m_unk1 = bReader.read<u8>();
		m_unk2 = bReader.read<u8>();
		m_unk3 = bReader.read<u8>();
		m_unk4 = bReader.read<u8>();
		m_unk5 = bReader.read<u8>();
		m_unk6 = bReader.read<u8>();
		m_unk7 = bReader.read<u8>();
		m_unk8 = bReader.read<u8>();
		m_unk9 = bReader.read<u8>();
		m_unk10 = bReader.read<u8>();
		m_unk11 = bReader.read<u8>();
		m_unk12 = bReader.read<u8>();
	}
};
struct PVWTevStage
{
	u8 m_unk1;
	u8 m_unk2;
	u8 m_unk3;
	u8 m_unk4;
	u8 m_unk5;
	u8 m_unk6;
	PVWCombiner m_combinerUnk1;
	PVWCombiner m_combinerUnk2;

	void read(oishii::BinaryReader& bReader)
	{
		m_unk1 = bReader.read<u8>(); // 1
		m_unk2 = bReader.read<u8>(); // 2
		m_unk3 = bReader.read<u8>(); // 3
		m_unk4 = bReader.read<u8>(); // 4
		m_unk5 = bReader.read<u8>(); // 5
		m_unk6 = bReader.read<u8>(); // 6

		// These two aren't assigned anything
		bReader.read<u8>(); // 7
		bReader.read<u8>(); // 8

		m_combinerUnk1.read(bReader); // 20
		m_combinerUnk2.read(bReader); // 32
	}
};
struct UnNamedClass4
{
	u32 m_unk1;
	PVWKeyInfoS10 m_unk2;
	void read(oishii::BinaryReader& bReader)
	{
		m_unk1 = bReader.read<u32>();
		m_unk2.read(bReader);
	}
};
struct UnNamedClass3
{
	std::vector<UnNamedClass4> m_unk1;
	void read(oishii::BinaryReader& bReader)
	{
		m_unk1.resize(bReader.read<u32>());
		if (m_unk1.size())
		{
			for (auto& it : m_unk1)
				it.read(bReader);
		}
	}
};
struct UnNamedClass2
{
	u32 m_unk1;
	PVWKeyInfoS10 m_unk2;
	PVWKeyInfoS10 m_unk3;
	PVWKeyInfoS10 m_unk4;
	void read(oishii::BinaryReader& bReader)
	{
		m_unk1 = bReader.read<u32>();
		m_unk2.read(bReader);
		m_unk3.read(bReader);
		m_unk4.read(bReader);
	}
};
struct UnNamedClass1
{
	std::vector<UnNamedClass2> m_unk1;
	void read(oishii::BinaryReader& bReader)
	{
		m_unk1.resize(bReader.read<u32>());
		if (m_unk1.size())
		{
			for (auto& it : m_unk1)
				it.read(bReader);
		}
	}
};
struct PVWTevColReg
{
	ShortColour m_colour;
	u32 m_unk1;
	f32 m_unk2;
	UnNamedClass1 m_unk3;
	UnNamedClass3 m_unk4;

	void read(oishii::BinaryReader& bReader)
	{
		m_colour << bReader;
		m_unk1 = bReader.read<u32>();
		m_unk2 = bReader.read<f32>();
		m_unk3.read(bReader);
		m_unk4.read(bReader);
	}
};

struct PVWTevInfo
{
	PVWTevColReg m_unk1;
	PVWTevColReg m_unk2;
	PVWTevColReg m_unk3;
	Colour m_CPREV;
	Colour m_CREG0;
	Colour m_CREG1;
	Colour m_CREG2;

	u32 m_tevStageCount = 0;
	std::vector<PVWTevStage> m_tevStages;

	PVWTevInfo() = default;
	~PVWTevInfo() = default;

	static void onRead(oishii::BinaryReader& bReader, PVWTevInfo& context)
	{
		context.m_unk1.read(bReader);
		context.m_unk2.read(bReader);
		context.m_unk3.read(bReader);
		context.m_CPREV << bReader;
		context.m_CREG0 << bReader;
		context.m_CREG1 << bReader;
		context.m_CREG2 << bReader;

		context.m_tevStageCount = bReader.read<u32>();
		if (context.m_tevStageCount)
		{
			context.m_tevStages.resize(context.m_tevStageCount);
			for (auto& tevStage : context.m_tevStages)
				tevStage.read(bReader);
		}
	}
};

struct Material
{
	u32 m_flag = 0;
	u32 m_textureIndex = 0;

	void read(oishii::BinaryReader& bReader)
	{
		m_flag = bReader.read<u32>();
		m_textureIndex = bReader.read<u32>();
		/* m_pCI.m_diffuseColour << bReader;
		if (this->m_flag & 1)
		{

		}
		*/
	}
	
	Material() = default;
	~Material() = default;
};

}

}
