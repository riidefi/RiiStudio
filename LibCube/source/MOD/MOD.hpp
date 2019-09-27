#pragma once

#include "MOD_Includes.hpp"
#include "TXE/TXE.hpp"

namespace libcube {

namespace pikmin1 {

enum MODCHUNKS
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

	std::vector<Vector3> m_vertices; // vertices
	std::vector<Vector3> m_vnorms; // vertex normals
	std::vector<NBT> m_nbt; // unknown, to do with vertices
	std::vector<Colour> m_colours; // vertex colours
	std::vector<VtxMatrix> m_vtxmatrices; // vertex matrices
	std::vector<Batch> m_batches; // meshes
	std::vector<Joint> m_joints; // joints
	std::vector<String> m_jointNames; // joint names
	std::vector<TXE> m_textures; // textures
	std::vector<Vector2> m_texcoords[8]; // texture coordinates 0 - 7, x = s, y  = t
	std::vector<TexAttr> m_texattrs; // texture attributes
	std::vector<Material> m_materials; // texture environments
	std::vector<Envelope> m_envelopes; // skinning envelopes

	std::vector<BaseCollTriInfo> m_baseCollTriInfo;
	std::vector<BaseRoomInfo> m_baseRoomInfo;
	CollGroup m_collisionGrid;

	// Reading
	void read_header(oishii::BinaryReader&);

	void read_vertices(oishii::BinaryReader&); // opcode 0x10
	void read_vertexnormals(oishii::BinaryReader&); // opcode 0x11
	void read_nbts(oishii::BinaryReader&); // opcode 0x12
	void read_vertexcolours(oishii::BinaryReader&); // opcode 0x13

	void read_texcoords(oishii::BinaryReader&, u32); // opcode 0x18 - 0x1F
	void read_textures(oishii::BinaryReader&); // opcode 0x20
	void read_texattr(oishii::BinaryReader&); // opcode 0x22
	void read_materials(oishii::BinaryReader&); // opcode 0x30

	void read_vtxmatrix(oishii::BinaryReader&); // opcode 0x40
	void read_envelope(oishii::BinaryReader&); // opcode 0x41
	void read_faces(oishii::BinaryReader&); // opcode 0x50
	void read_joints(oishii::BinaryReader&); // opcode 0x60
	void read_jointnames(oishii::BinaryReader&); // opcode 0x61
	void read_basecolltriinfo(oishii::BinaryReader&); // opcode 0x100
	void read_collisiongrid(oishii::BinaryReader&); // opcode 0x110

	MOD() = default;
	~MOD() = default;

	void read(oishii::BinaryReader&);
};

}

}
