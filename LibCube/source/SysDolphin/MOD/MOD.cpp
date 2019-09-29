#define OISHII_ALIGNMENT_CHECK 0
#include "MOD.hpp"

namespace libcube { namespace pikmin1 {


void MOD::read_header(oishii::BinaryReader& bReader)
{
	skipPadding(bReader);

	m_header.m_year = bReader.read<u16>();
	m_header.m_month = bReader.read<u8>();
	m_header.m_day = bReader.read<u8>();
	// unsure as to what m_unk is, changes from file to file
	m_header.m_unk = bReader.read<u32>();
	DebugReport("Creation date of model file (YYYY/MM/DD): %u/%u/%u\n", m_header.m_year, m_header.m_month, m_header.m_day);

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

void MOD::parse(oishii::BinaryReader& bReader)
{	
	bReader.setEndian(true); // big endian

	u32 cDescriptor = 0;

	while ((cDescriptor = bReader.read<u32>()) != (u16)-1)
	{
		const u32 cStart = bReader.tell() - 4; // get the offset of the chunk start
		const u32 cLength = bReader.read<u32>();

		if (cStart & 0x1f)
		{
			bReader.warnAt("bReader.tell() isn't aligned with 0x20! ERROR!\n", cStart - 4 , cStart);
			return;
		}

		switch (static_cast<MODCHUNKS>(cDescriptor))
		{
		case MODCHUNKS::MOD_HEADER:
			read_header(bReader);
			break;
		case MODCHUNKS::MOD_VERTEX:
			readChunk(bReader, m_vertices);
			break;
		case MODCHUNKS::MOD_VERTEXNORMAL:
			readChunk(bReader, m_vnorms);
			break;
		case MODCHUNKS::MOD_NBT:
			readChunk(bReader, m_nbt);
			break;
		case MODCHUNKS::MOD_VERTEXCOLOUR:
			readChunk(bReader, m_colours);
			break;
		case MODCHUNKS::MOD_TEXCOORD0:
		case MODCHUNKS::MOD_TEXCOORD1:
		case MODCHUNKS::MOD_TEXCOORD2:
		case MODCHUNKS::MOD_TEXCOORD3:
		case MODCHUNKS::MOD_TEXCOORD4:
		case MODCHUNKS::MOD_TEXCOORD5:
		case MODCHUNKS::MOD_TEXCOORD6:
		case MODCHUNKS::MOD_TEXCOORD7:
			readChunk(bReader, m_texcoords[cDescriptor - (int)MODCHUNKS::MOD_TEXCOORD0]);
			break;
		case MODCHUNKS::MOD_TEXTURE:
			readChunk(bReader, m_textures);
			break;
		case MODCHUNKS::MOD_TEXTURE_ATTRIBUTE:
			readChunk(bReader, m_texattrs);
			break;
		case MODCHUNKS::MOD_VTXMATRIX:
			readChunk(bReader, m_vtxmatrices);
			break;
		case MODCHUNKS::MOD_ENVELOPE:
			readChunk(bReader, m_envelopes);
			break;
		case MODCHUNKS::MOD_MESH:
			readChunk(bReader, m_batches);
			break;
		case MODCHUNKS::MOD_JOINT:
			readChunk(bReader, m_joints);
			break;
		case MODCHUNKS::MOD_JOINT_NAME:
			readChunk(bReader, m_jointNames);
			break;
		case MODCHUNKS::MOD_COLLISION_TRIANGLE:
			read_basecolltriinfo(bReader);
			break;
		case MODCHUNKS::MOD_COLLISION_GRID:
			m_collisionGrid << bReader;
			break;
		case MODCHUNKS::MOD_EOF: // caught because it's not a valid chunk to read, so don't even bother warning user and just break
			break;
		default:
			bReader.warnAt("Unimplemented chunk\n", cStart, cStart + 4);
			skipChunk(bReader, cLength);
			break;
		}
	}

	if (bReader.tell() != bReader.endpos())
	{
		DebugReport("INI file found at end of file\n");
	}

	DebugReport("Done reading file\n");
	removeMtxDependancy();
}

void MOD::removeMtxDependancy()
{
	for (auto& batch : m_batches)
	{
		batch.m_depMTXGroups = 0;
		for (auto& mtx_group : batch.m_mtxGroups)
		{
			mtx_group.m_dependant.clear();
		}
	}
}

} } // libcube::pikmin1
