#pragma once

#include "../allincludes.hpp"
#include <LibCube/Pikmin1/TXE/TXE.hpp>

namespace libcube { namespace pikmin1 {



struct MOD
{
	enum class Chunks : u16
	{
		Header = 0x0000,

		VertexPosition = 0x0010,
		VertexNormal = 0x0011,
		VertexNBT = 0x0012,
		VertexColor = 0x0013,

		VertexUV0 = 0x0018,
		VertexUV1 = 0x0019,
		VertexUV2 = 0x001A,
		VertexUV3 = 0x001B,
		VertexUV4 = 0x001C,
		VertexUV5 = 0x001D,
		VertexUV6 = 0x001E,
		VertexUV7 = 0x001F,

		Texture = 0x0020,
		TextureAttribute = 0x0022,
		Material = 0x0030,

		VertexMatrix = 0x0040,

		Envelope = 0x0041,

		Mesh = 0x0050,

		Joint = 0x0060,
		JointName = 0x0061,

		CollisionPrism = 0x0100,
		CollisionGrid = 0x0110,

		EoF = 0xFFFF
	};

	struct
	{
		u16 m_year = 0;
		u8 m_month = 0;
		u8 m_day = 0;
		u32 m_systemUsed = 0;
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
				it << reader;

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
		static constexpr char tevinfos[] = "Texture Environments";
		static constexpr char envelope[] = "Skinning Envelope";
	};
	static constexpr char name[] = "Pikmin 1 MOD file";

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

	Holder<Material, Names::materials> m_materials;
	Holder<PVWTevInfo, Names::tevinfos> m_tevinfos;

	Holder<Envelope, Names::envelope> m_envelopes; // skinning envelopes

	std::vector<BaseCollTriInfo> m_baseCollTriInfo;
	std::vector<BaseRoomInfo> m_baseRoomInfo;
	CollGroup m_collisionGrid;
	u32 m_numJoints = 0;

	// Reading
	void read_header(oishii::BinaryReader&);

	void read_basecolltriinfo(oishii::BinaryReader&); // opcode 0x100
	void read_materials(oishii::BinaryReader&); // opcode 0x30

	MOD() = default;
	~MOD() = default;

	static void onRead(oishii::BinaryReader&, MOD&);
	void removeMtxDependancy();
};
inline void operator<<(MOD& context, oishii::BinaryReader& bReader)
{
	bReader.dispatch<MOD, oishii::Direct, false>(context);
}

}

}
