#pragma once

#include <oishii/reader/binary_reader.hxx>
#include <vector>
#include <glm/glm.hpp>

namespace libcube {

namespace pikmin1 {

static inline void skipChunk(oishii::BinaryReader& bReader, u32 offset)
{
	bReader.seek<oishii::Whence::Current>(offset);
}

static inline void skipPadding(oishii::BinaryReader& bReader)
{
	const u32 currentPos = bReader.tell();
	const u32 toRead = (~(0x20 - 1) & (currentPos + 0x20 - 1)) - currentPos;
	skipChunk(bReader, toRead);
}

inline static void MOD_readVec3(oishii::BinaryReader & bReader, glm::vec3 & vector3)
{
	vector3.x = bReader.read<float>();
	vector3.y = bReader.read<float>();
	vector3.z = bReader.read<float>();
}

inline static void MOD_readVec2(oishii::BinaryReader & bReader, glm::vec2 & vector2)
{
	vector2.x = bReader.read<float>();
	vector2.y = bReader.read<float>();
}

//! @brief Unsure of purpose, used in BaseCollTriInfo
struct Plane
{
	constexpr static const char name[] = "Plane";
	glm::vec3 m_unk1;
	f32 m_unk2;

	static void onRead(oishii::BinaryReader& bReader, Plane& context)
	{
		MOD_readVec3(bReader, context.m_unk1);
		context.m_unk2 = bReader.read<f32>();
	}
};

//! @brief Base Collision Triangle Information
struct BaseCollTriInfo
{
	constexpr static const char name[] = "Base Collision Triangle Information";
	u32 m_unk1;
	u32 m_unk2;
	u32 m_unk3;
	u32 m_unk4;

	u16 m_unk5;
	u16 m_unk6;
	u16 m_unk7;
	u16 m_unk8;

	Plane m_unk9;

	static void onRead(oishii::BinaryReader& bReader, BaseCollTriInfo& context)
	{
		context.m_unk1 = bReader.read<u32>();
		context.m_unk2 = bReader.read<u32>();
		context.m_unk3 = bReader.read<u32>();
		context.m_unk4 = bReader.read<u32>();

		context.m_unk5 = bReader.read<u16>();
		context.m_unk6 = bReader.read<u16>();
		context.m_unk7 = bReader.read<u16>();
		context.m_unk8 = bReader.read<u16>();

		context.m_unk9.onRead(bReader, context.m_unk9);
	}
};

//! @brief Unsure of purpose, used in BCTI chunk (0x)
struct BaseRoomInfo
{
	constexpr static const char name[] = "Base Room Information";
	u32 m_unk1;

	static void onRead(oishii::BinaryReader& bReader, BaseRoomInfo& context)
	{
		context.m_unk1 = bReader.read<u32>();
	}
};

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

struct Colour
{
	u8 m_R, m_G, m_B, m_A;
	Colour() { m_R = m_G = m_B = m_A = 0; }
	Colour(u8 _r, u8 _g, u8 _b, u8 _a) : m_R(_r), m_G(_g), m_B(_b), m_A(_a) {}

	void read(oishii::BinaryReader& bReader)
	{
		m_R = bReader.read<u8>();
		m_G = bReader.read<u8>();
		m_B = bReader.read<u8>();
		m_A = bReader.read<u8>();
	}
};

//! @brief Material
struct Material
{
	u32 m_hasPE; // pixel engine
	u32 m_unk2;
	Colour m_unkColour1;

	// if (hasPE & 1)
	u32 m_unk3;

	void read(oishii::BinaryReader& bReader)
	{
		m_hasPE = bReader.read<u32>();
		m_unk2 = bReader.read<u32>();
		m_unkColour1.read(bReader);

		if (m_hasPE & 1)
		{
			m_unk3 = bReader.read<u32>();
			// PVWPolygonColourInfo
			// PVWLightingInfo
			// PVWPeInfo
			// PVWTextureInfo
		}
	}
};

//! @brief DispList, contains u8 vector full of face data
struct DispList
{
	constexpr static const char name[] = "Display List";

	u32 m_unk1 = 0;
	u32 m_unk2 = 0;

	std::vector<u8> m_dispData;

	static void onRead(oishii::BinaryReader& bReader, DispList& context)
	{
		context.m_unk1 = bReader.read<u32>();
		context.m_unk2 = bReader.read<u32>();
		context.m_dispData.resize(bReader.read<u32>());

		skipPadding(bReader);
		// Read the displist data
		for (auto& dispData : context.m_dispData)
			dispData = bReader.read<u8>();
	}
};

//! @brief Matrix Group, contains DispList
struct MtxGroup
{
	constexpr static const char name[] = "Matrix Group";

	std::vector<u16> m_unk1Array;
	std::vector<DispList> m_dispLists;

	static void onRead(oishii::BinaryReader& bReader, MtxGroup& context)
	{
		// Unknown purpose of the array, store it anyways
		context.m_unk1Array.resize(bReader.read<u32>());
		for (auto& unkArr : context.m_unk1Array)
			unkArr = bReader.read<u16>();

		context.m_dispLists.resize(bReader.read<u32>());

		for (auto& dLists : context.m_dispLists)
		{
			bReader.dispatch<DispList, oishii::Direct, false>(dLists);
		}
	}
};

//! @brief Batch, contains Matrix Group
struct Batch
{
	constexpr static const char name[] = "Mesh Batch";

	u32 m_unk1 = 0;
	u32 m_vcd = 0;	//	Vertex Descriptor

	std::vector<MtxGroup> m_mtxGroups;

	static void onRead(oishii::BinaryReader& bReader, Batch& context)
	{
		// Read the batch variables
		context.m_unk1 = bReader.read<u32>();
		context.m_vcd = bReader.read<u32>(); // Vertex descriptor

		context.m_mtxGroups.resize(bReader.read<u32>());
		for (auto& mGroup : context.m_mtxGroups)
		{
			// Calls MtxGroup::onRead
			bReader.dispatch<MtxGroup, oishii::Direct, false>(mGroup);
		}
	}
};

//! @brief Joint, contains MatPoly
struct Joint
{
	constexpr static const char name[] = "Mesh Joint";

	u32 m_unk1;
	bool m_unk2;
	bool m_unk3;
	glm::vec3 m_unk4;
	glm::vec3 m_unk5;
	f32 m_unk6;
	glm::vec3 m_unk7;
	glm::vec3 m_unk8;
	glm::vec3 m_unk9;
	struct MatPoly
	{
		u16 m_unk1;
		u16 m_unk2;
	};
	std::vector<MatPoly> m_matpolys;


	static void onRead(oishii::BinaryReader& bReader, Joint& context)
	{
		context.m_unk1 = bReader.read<u32>();

		const u16 unk1 = static_cast<u16>(bReader.read<u32>());
		context.m_unk2 = (unk1 & 1) != 0;
		context.m_unk3 = (unk1 & 0x4000) != 0;

		MOD_readVec3(bReader, context.m_unk4);
		MOD_readVec3(bReader, context.m_unk5);
		context.m_unk6 = bReader.read<f32>();
		MOD_readVec3(bReader, context.m_unk7);
		MOD_readVec3(bReader, context.m_unk8);
		MOD_readVec3(bReader, context.m_unk9);

		context.m_matpolys.resize(bReader.read<u32>());
		for (auto& matpoly : context.m_matpolys)
		{
			matpoly.m_unk1 = bReader.read<u16>();
			matpoly.m_unk2 = bReader.read<u16>();
		}
	}
};

struct VtxMatrix
{
	constexpr static const char name[] = "Mesh VtxMatrix";

	bool m_unk1;
	u16 m_unk2;

	static void onRead(oishii::BinaryReader& bReader, VtxMatrix& context)
	{
		const s16 unk1 = bReader.read<s16>();
		context.m_unk2 = (context.m_unk1 = unk1 >= 0) ? unk1 : -unk1 - 1;
	}
};

struct UnkVec3s
{
	glm::vec3 m_1;
	glm::vec3 m_2;
	glm::vec3 m_3;
};

}

}
