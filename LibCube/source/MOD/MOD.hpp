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

namespace pikmin1 {
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

//! @brief Batch, contains MtxGroup
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

class MOD final
{
private:
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

	std::vector<Batch> m_batches; // meshs

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

	inline const std::vector <glm::vec3> vertices() const noexcept;
	inline const std::vector <glm::vec3> vertexNormals() const noexcept;
	inline const std::vector <Colour> colours() const noexcept;
	inline const std::vector<Batch> batches() const noexcept;
};

}

}

