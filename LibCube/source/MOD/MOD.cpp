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

void MOD::read_vnormals(oishii::BinaryReader& bReader)
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

void MOD::read_colours(oishii::BinaryReader& bReader)
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
		// calls Batch::onRead
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
	DebugReport("Reading texture co-ordinates\n");
	const u32 newIndex = opcode - 0x18;
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
		// calls BaseRoomInfo::onRead
		bReader.dispatch<BaseRoomInfo, oishii::Direct, false>(info);
	}

	skipPadding(bReader);

	for (auto& collTri : m_baseCollTriInfo)
	{
		// calls BaseCollTriInfo::onRead
		bReader.dispatch<BaseCollTriInfo, oishii::Direct, false>(collTri);
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
		// calls String::onRead
		bReader.dispatch<String, oishii::Direct, false>(str);
	}
	skipPadding(bReader);
}

void MOD::read(oishii::BinaryReader& bReader)
{
	u32 cDescriptor = 0;
	bReader.setEndian(true);	// big endian
	bReader.seekSet(0);		// reset file pointer position

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
		case MODCHUNKS::HEADER:
			read_header(bReader);
			break;
		case MODCHUNKS::VERTICES:
			read_vertices(bReader);
			break;
		case MODCHUNKS::VNORMALS:
			read_vnormals(bReader);
			break;
		case MODCHUNKS::MESHCOLOURS:
			read_colours(bReader);
			break;
		case MODCHUNKS::JOINT_NAMES:
			read_jointnames(bReader);
			break;
		case MODCHUNKS::MESH:
			read_faces(bReader);
			break;
		case MODCHUNKS::TEXCOORD0:
		case MODCHUNKS::TEXCOORD1:
		case MODCHUNKS::TEXCOORD2:
		case MODCHUNKS::TEXCOORD3:
		case MODCHUNKS::TEXCOORD4:
		case MODCHUNKS::TEXCOORD5:
		case MODCHUNKS::TEXCOORD6:
		case MODCHUNKS::TEXCOORD7:
			read_texcoords(bReader, cDescriptor);
			break;
		case MODCHUNKS::TEXTURES:
			read_textures(bReader);
			break;
		case MODCHUNKS::COLLISION_TRIANGLES:
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
