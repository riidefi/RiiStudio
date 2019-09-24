#pragma once

#include <oishii/reader/binary_reader.hxx>
#include <vector>
#include <glm/glm.hpp>
#include "TXE/TXE.hpp"

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

namespace libcube {

namespace pikmin1 {

inline static void MOD_readVec3(oishii::BinaryReader& bReader, glm::vec3& vector3)
{
	vector3.x = bReader.read<float>();
	vector3.y = bReader.read<float>();
	vector3.z = bReader.read<float>();
}

inline static void MOD_readVec2(oishii::BinaryReader& bReader, glm::vec2& vector2)
{
	vector2.x = bReader.read<float>();
	vector2.y = bReader.read<float>();
}

enum MODCHUNKS
{
	HEADER = 0x0000,
	VERTICES = 0x0010,
	VNORMALS = 0x0011,
	UNKVEC3FS = 0x0012,
	MESHCOLOURS = 0x0013,
	TEXCOORD0 = 0x0018,
	TEXCOORD1 = 0x0019,
	TEXCOORD2 = 0x001A,
	TEXCOORD3 = 0x001B,
	TEXCOORD4 = 0x001C,
	TEXCOORD5 = 0x001D,
	TEXCOORD6 = 0x001E,
	TEXCOORD7 = 0x001F,
	TEXTURES = 0x0020,
	MIPMAPS = 0x0022,
	MATERIALS = 0x0030,
	VTXMATRIX = 0x0040,
	ENVELOPE = 0x0041,
	MESH = 0x0050,
	SKELETON = 0x0060,
	JOINT_NAMES = 0x0061,
	COLLISION_TRIANGLES = 0x0100,
	BOUNDINGBOX = 0x0110,
};

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

struct MOD
{
	struct
	{
		u8 m_PADDING0[0x18];	// 24 bytes
		u16 m_year = 0;	// 26 bytes
		u8 m_month = 0;	// 27 bytes
		u8 m_day = 0;	// 28 bytes
		int m_unk = 0;	// 32 bytes
		u8 m_PADDING1[0x18];	// 56 bytes
	} m_header; // header will always be 56 bytes

	std::vector<glm::vec3> m_vertices;
	std::vector<glm::vec3> m_vnorms;
	std::vector<Colour> m_colours;
	std::vector<String> m_jointNames;
	std::vector<Batch> m_batches; // meshes
	std::vector<TXE> m_textures;
	std::vector<glm::vec2> m_texcoords[8];

	std::vector<BaseCollTriInfo> m_baseCollTriInfo;
	std::vector<BaseRoomInfo> m_baseRoomInfo;

	// Reading
	void read_header(oishii::BinaryReader&);
	void read_vertices(oishii::BinaryReader&);
	void read_vnormals(oishii::BinaryReader&);
	void read_colours(oishii::BinaryReader&);
	void read_faces(oishii::BinaryReader&);
	void read_textures(oishii::BinaryReader&);
	void read_texcoords(oishii::BinaryReader&, u32);
	void read_basecolltriinfo(oishii::BinaryReader&);

	void read_jointnames(oishii::BinaryReader&);

	MOD() = default;
	~MOD() = default;

	void read(oishii::BinaryReader&);
};

}

}
