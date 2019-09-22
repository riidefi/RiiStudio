#pragma once

#include <oishii/reader/binary_reader.hxx>
#include <vector>
#include <glm/glm.hpp>


namespace libcube {

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

	u32 m_vertexCount;
	std::vector<glm::vec3> m_vertices;

	u32 m_vNormalCount;
	std::vector<glm::vec3> m_vnorms;

	u32 m_colourCount;
	std::vector<Colour> m_colours;

	// Reading
	inline void read_header(oishii::BinaryReader&);
	inline void read_vertices(oishii::BinaryReader&);
	inline void read_vnormals(oishii::BinaryReader&);
	inline void read_colours(oishii::BinaryReader&);

	inline void skipPadding(oishii::BinaryReader&);
	inline void skipChunk(oishii::BinaryReader&, u32);
public:
	MOD() = default;
	~MOD() = default;

	void read(oishii::BinaryReader&);
};
}
