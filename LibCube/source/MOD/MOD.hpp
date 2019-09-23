#pragma once

#include <oishii/reader/binary_reader.hxx>
#include <vector>
#include <glm/glm.hpp>

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

namespace pikmin1mesh
{

struct DispList
{
	u32 m_dword14;
	u32 m_dispListSize;
	std::vector<s8> m_dispListData;
	u32 m_dword20;
	u32 m_dword24;
	u32 m_dword28;

	void read(oishii::BinaryReader& bReader)
	{
		m_dword14 = bReader.read<u32>();
		m_dword28 = bReader.read<u32>();

		m_dispListSize = bReader.read<u32>();
		skipPadding(bReader);
		m_dispListData.resize(m_dispListSize);
		for (u32 i = 0; i < m_dispListSize; i++)
			m_dispListData[i] = bReader.read<s8>();
	}
};

struct MtxGroup
{
	u32 m_unkCount;
	s32 m_dispListCount;
	DispList* m_dispLists;

	void read(oishii::BinaryReader& bReader)
	{
		m_unkCount = bReader.read<u32>();
		for (int i = 0; i < m_unkCount; i++)
			bReader.read<short>();
		m_dispListCount = bReader.read<s32>();
		if (m_dispListCount)
		{
			m_dispLists = new DispList[m_dispListCount];
			for (int i = 0; i < m_unkCount; i++)
				m_dispLists[i].read(bReader);
		}
	}
};

struct Mesh
{
	u32 m_dword14;
	u32 m_unk1;
	u32 m_dword1C;
	u32 m_mtxGroupCount;
	MtxGroup* m_mtxGroups;
	u32 m_dword28;
	u32 m_vcd;

	Mesh() = default;
	~Mesh() { delete[] m_mtxGroups; }

	void read(oishii::BinaryReader& bReader)
	{
		m_unk1 = bReader.read<u32>();
		m_vcd = bReader.read<u32>();
		m_mtxGroupCount = bReader.read<u32>();
		if (m_mtxGroupCount)
		{
			m_mtxGroups = new MtxGroup[m_mtxGroupCount];
			for (u32 i = 0; i < m_mtxGroupCount; i++)
				m_mtxGroups[i].read(bReader);
		}
	}
};

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

class MOD final
{
private:
	struct
	{
		u8 m_PADDING0[0x18];	// 24 bytes
		u16 m_year;	// 26 bytes
		u8 m_month;	// 27 bytes
		u8 m_day;	// 28 bytes
		int m_unk;	// 32 bytes
		u8 m_PADDING1[0x18];	// 56 bytes
	} m_header; // header will always be 56 bytes


	u32 m_vertexCount;
	std::vector<glm::vec3> m_vertices;

	u32 m_vNormalCount;
	std::vector<glm::vec3> m_vnorms;

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
	u32 m_colourCount;
	std::vector<Colour> m_colours;

	u32 m_jointNameCount;
	std::vector<std::string> m_jointNames;

	u32 m_batchCount;
	std::vector<pikmin1mesh::Mesh> m_batches;

	// Reading
	inline void read_header(oishii::BinaryReader&);
	inline void read_vertices(oishii::BinaryReader&);
	inline void read_vnormals(oishii::BinaryReader&);
	inline void read_colours(oishii::BinaryReader&);
	inline void read_faces(oishii::BinaryReader&);

	inline void read_jointnames(oishii::BinaryReader&);
public:
	MOD() = default;
	~MOD() = default;

	void read(oishii::BinaryReader&);
};

}

