#define OISHII_ALIGNMENT_CHECK 0
#include "MOD.hpp"
#include <common.hpp>

namespace libcube {

namespace pikmin1 {

void MOD::read_header(oishii::BinaryReader& bReader)
{
	skipPadding(bReader);

	m_header.m_year = bReader.read<u16>();
	m_header.m_month = bReader.read<u8>();
	m_header.m_day = bReader.read<u8>();
	m_header.m_unk = bReader.read<u32>();
	DebugReport("Creation date of model file (YYYY/MM/DD): %u/%u/%u\n", m_header.m_year, m_header.m_month, m_header.m_day);

	skipPadding(bReader);
}

void MOD::read_vertices(oishii::BinaryReader& bReader)
{
	DebugReport("Reading vertices\n");
	m_vertices.resize(bReader.read<u32>());

	skipPadding(bReader);
	for (auto& vertex : m_vertices)
	{
		MOD_readVec3(bReader, vertex);
	}
	skipPadding(bReader);
}

void MOD::read_vertexnormals(oishii::BinaryReader& bReader)
{
	DebugReport("Reading vertex normals\n");
	m_vnorms.resize(bReader.read<u32>());

	skipPadding(bReader);
	for (auto& vnorm : m_vnorms)
	{
		MOD_readVec3(bReader, vnorm);
	}
	skipPadding(bReader);
}

void MOD::read_unknownvector(oishii::BinaryReader& bReader)
{
	DebugReport("Reading unknown set of 3 vector3s\n");
	m_unkvert.resize(bReader.read<u32>());

	skipPadding(bReader);
	for (auto& vert : m_unkvert)
	{
		MOD_readVec3(bReader, vert.m_1);
		MOD_readVec3(bReader, vert.m_2);
		MOD_readVec3(bReader, vert.m_3);
		/*std::printf("NEW SET %f %f %f\n %f %f %f\n %f %f %f\n", vert.m_1.x, vert.m_1.y, vert.m_1.z,
																									vert.m_2.x, vert.m_2.y, vert.m_2.z,
																									vert.m_3.x, vert.m_3.y, vert.m_3.z);*/
	}
	skipPadding(bReader);
}

void MOD::read_vertexcolours(oishii::BinaryReader& bReader)
{
	DebugReport("Reading mesh colours\n");
	m_colours.resize(bReader.read<u32>());

	skipPadding(bReader);
	for (auto& colour : m_colours)
	{
		colour.read(bReader);
	}
	skipPadding(bReader);
}

void MOD::read_faces(oishii::BinaryReader& bReader)
{
	DebugReport("Reading faces\n");
	m_batches.resize(bReader.read<u32>());

	skipPadding(bReader);
	for (auto& cBatch : m_batches)
	{
		bReader.dispatch<Batch, oishii::Direct, false>(cBatch);
	}
	skipPadding(bReader);
}

void MOD::read_textures(oishii::BinaryReader& bReader)
{
	DebugReport("Reading textures\n");
	m_textures.resize(bReader.read<u32>());

	skipPadding(bReader);
	for (auto& texture : m_textures)
	{
		texture.readModFile(bReader);
	}
	skipPadding(bReader);
}

void MOD::read_texcoords(oishii::BinaryReader& bReader, u32 opcode)
{
	const u32 newIndex = opcode - 0x18;
	DebugReport("Reading texcoord%d\n", newIndex);
	m_texcoords[newIndex].resize(bReader.read<u32>());

	skipPadding(bReader);
	for (auto& coords : m_texcoords[newIndex])
	{
		MOD_readVec2(bReader, coords);
	}
	skipPadding(bReader);
}

void MOD::read_basecolltriinfo(oishii::BinaryReader& bReader)
{
	DebugReport("Reading collision triangle information\n");
	m_baseCollTriInfo.resize(bReader.read<u32>());
	m_baseRoomInfo.resize(bReader.read<u32>());

	skipPadding(bReader);
	for (auto& info : m_baseRoomInfo)
	{
		bReader.dispatch<BaseRoomInfo, oishii::Direct, false>(info);
	}

	skipPadding(bReader);
	for (auto& collTri : m_baseCollTriInfo)
	{
		bReader.dispatch<BaseCollTriInfo, oishii::Direct, false>(collTri);
	}

	skipPadding(bReader);
}

void MOD::read_vtxmatrix(oishii::BinaryReader& bReader)
{
	DebugReport("Reading vertex matrix\n");
	m_vtxmatrices.resize(bReader.read<u32>());

	skipPadding(bReader);
	for (auto& vtxmatrix : m_vtxmatrices)
	{
		bReader.dispatch<VtxMatrix, oishii::Direct, false>(vtxmatrix);
	}
	skipPadding(bReader);
}

void MOD::read_joints(oishii::BinaryReader& bReader)
{
	DebugReport("Reading joints\n");
	m_joints.resize(bReader.read<u32>());

	skipPadding(bReader);
	for (auto& joint : m_joints)
	{
		bReader.dispatch<Joint, oishii::Direct, false>(joint);
	}
	skipPadding(bReader);
}

void MOD::read_jointnames(oishii::BinaryReader& bReader)
{
	DebugReport("Reading joint names\n");
	m_jointNames.resize(bReader.read<u32>());

	skipPadding(bReader);
	for (auto& str : m_jointNames)
	{
		bReader.dispatch<String, oishii::Direct, false>(str);
	}
	skipPadding(bReader);
}

void MOD::read(oishii::BinaryReader& bReader)
{
	u32 cDescriptor = 0;
	bReader.setEndian(true); // big endian
	bReader.seekSet(0); // reset file pointer position

	do
	{
		const u32 cPosition = bReader.tell();
		cDescriptor = bReader.read<u32>();
		const u32 cLength = bReader.read<u32>();

		if (cPosition & 0x1f)
		{
			DebugReport("bReader.tell() isn't aligned with 0x20! ERROR!\n");
			return;
		}

		switch (cDescriptor)
		{
		case MODCHUNKS::MOD_HEADER:
			read_header(bReader);
			break;
		case MODCHUNKS::MOD_VERTEX:
			read_vertices(bReader);
			break;
		case MODCHUNKS::MOD_VERTEXNORMAL:
			read_vertexnormals(bReader);
			break;
		case MODCHUNKS::MOD_UNKVEC3F:
			read_unknownvector(bReader);
			break;
		case MODCHUNKS::MOD_VERTEXCOLOUR:
			read_vertexcolours(bReader);
			break;

		case MODCHUNKS::MOD_TEXCOORD0:
		case MODCHUNKS::MOD_TEXCOORD1:
		case MODCHUNKS::MOD_TEXCOORD2:
		case MODCHUNKS::MOD_TEXCOORD3:
		case MODCHUNKS::MOD_TEXCOORD4:
		case MODCHUNKS::MOD_TEXCOORD5:
		case MODCHUNKS::MOD_TEXCOORD6:
		case MODCHUNKS::MOD_TEXCOORD7:
			read_texcoords(bReader, cDescriptor);
			break;
		case MODCHUNKS::MOD_TEXTURE:
			read_textures(bReader);
			break;

		case MODCHUNKS::MOD_VTXMATRIX:
			read_vtxmatrix(bReader);
			break;
		case MODCHUNKS::MOD_MESH:
			read_faces(bReader);
			break;
		case MODCHUNKS::MOD_JOINT:
			read_joints(bReader);
			break;
		case MODCHUNKS::MOD_JOINT_NAME:
			read_jointnames(bReader);
			break;

		case MODCHUNKS::MOD_COLLISION_TRIANGLE:
			read_basecolltriinfo(bReader);
			break;
		default:
			DebugReport("Got chunk %04x, not implemented yet!\n", cDescriptor);
			skipChunk(bReader, cLength);
			break;
		}
	} while (cDescriptor != 0xFFFF);
}

} // pikmin1

} // libcube
