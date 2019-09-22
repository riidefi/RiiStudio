#pragma once

#include "../allincludes.hpp"
#include "SysDolphin/TXE/TXE.hpp"

namespace libcube { namespace pikmin1 {

enum class MODCHUNKS : u16
{
	MOD_HEADER = 0x0000,

	MOD_VERTEX = 0x0010,
	MOD_VERTEXNORMAL = 0x0011,
	MOD_NBT = 0x0012,
	MOD_VERTEXCOLOUR = 0x0013,

	MOD_TEXCOORD0 = 0x0018,
	MOD_TEXCOORD1 = 0x0019,
	MOD_TEXCOORD2 = 0x001A,
	MOD_TEXCOORD3 = 0x001B,
	MOD_TEXCOORD4 = 0x001C,
	MOD_TEXCOORD5 = 0x001D,
	MOD_TEXCOORD6 = 0x001E,
	MOD_TEXCOORD7 = 0x001F,

	MOD_TEXTURE = 0x0020,
	MOD_TEXTURE_ATTRIBUTE = 0x0022,
	MOD_MATERIAL = 0x0030,

	MOD_VTXMATRIX = 0x0040,
	MOD_ENVELOPE = 0x0041,
	MOD_MESH = 0x0050,
	MOD_JOINT = 0x0060,
	MOD_JOINT_NAME = 0x0061,
	MOD_COLLISION_TRIANGLE = 0x0100,
	MOD_COLLISION_GRID = 0x0110,
	MOD_EOF = 0xFFFF
};

struct MOD
{
	struct
	{
		u16 m_year = 0;
		u8 m_month = 0;
		u8 m_day = 0;
		int m_unk = 0;
	} m_header;

	template<typename T, const char N[]>
	struct Holder : public std::vector<T>
	{
		static constexpr const char* name = N;

		static void onRead(oishii::BinaryReader& reader, Holder& out)
		{
			out.resize(reader.read<u32>());

			skipPadding(reader);

			for (auto& it : out)
				read(reader, it);

			skipPadding(reader);
		}
	};

	struct Names
	{
		static constexpr char vertices[] = "Vertices";
		static constexpr char normals[] = "Normals";
		static constexpr char nbt[] = "NBT";
		static constexpr char colours[] = "Colors";
		static constexpr char vtxmatrices[] = "Vertex Matrices";
		static constexpr char batches[] = "Batches";
		static constexpr char joints[] = "Joints";
		static constexpr char jointnames[] = "Joint Names";
		static constexpr char textures[] = "Textures";
		static constexpr char texcoords[] = "Texture Coordinates";
		static constexpr char texattribs[] = "Texture Attributes";
		static constexpr char materials[] = "Materials";
		static constexpr char envelope[] = "Skinning Envelope";
	};

	Holder<glm::vec3, Names::vertices> m_vertices; // vertices
	Holder<glm::vec3, Names::normals> m_vnorms; // vertex normals
	Holder<NBT, Names::nbt> m_nbt; // vertex nbt vectors
	Holder<Colour, Names::colours> m_colours; // vertex colours
	Holder<VtxMatrix, Names::vtxmatrices> m_vtxmatrices; // vertex matrices
	Holder<Batch, Names::batches> m_batches; // meshes
	Holder<Joint, Names::joints> m_joints; // joints
	Holder<String, Names::jointnames> m_jointNames; // joint names
	Holder<TXE, Names::textures> m_textures; // textures
	Holder<glm::vec2, Names::texcoords> m_texcoords[8]; // texture coordinates 0 - 7, x = s, y  = t
	Holder<TexAttr, Names::texattribs> m_texattrs; // texture attributes
	Holder<Material, Names::materials> m_materials; // texture environments
	Holder<Envelope, Names::envelope> m_envelopes; // skinning envelopes

	std::vector<BaseCollTriInfo> m_baseCollTriInfo;
	std::vector<BaseRoomInfo> m_baseRoomInfo;
	CollGroup m_collisionGrid;

	// Reading
	void read_header(oishii::BinaryReader&);

	void read_basecolltriinfo(oishii::BinaryReader&); // opcode 0x100
	void read_collisiongrid(oishii::BinaryReader&); // opcode 0x110

	MOD() = default;
	~MOD() = default;

	void parse(oishii::BinaryReader&);
};

}

}
