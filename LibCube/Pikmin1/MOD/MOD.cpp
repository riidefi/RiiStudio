#include "MOD.hpp"

namespace libcube {
namespace pikmin1 {


void MOD::read_header(oishii::BinaryReader& bReader)
{
	skipPadding(bReader);

	m_header.m_year = bReader.read<u16>();
	m_header.m_month = bReader.read<u8>();
	m_header.m_day = bReader.read<u8>();
	DebugReport("Creation date of model file (YYYY/MM/DD): %u/%u/%u\n", m_header.m_year, m_header.m_month, m_header.m_day);
	// Used to identify what kind of system is being used,
	// Can be used to identify if using NBT or Classic/Softimage scaling system
	m_header.m_systemUsed = bReader.read<u32>();
	const bool usingEmboss = m_header.m_systemUsed & 1;
	const bool whichScaling = m_header.m_systemUsed & 8;
	if (usingEmboss)
		DebugReport("Model file using Emboss NBT!\n");
	if (whichScaling)
		DebugReport("Model file using Classic Scaling System!\n");
	else
		DebugReport("Model file using SoftImage Scaling System!\n");

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

void MOD::read_materials(oishii::BinaryReader& bReader)
{
	DebugReport("Reading materials\n");

}

void MOD::onRead(oishii::BinaryReader& bReader, MOD& context)
{
	bReader.setEndian(true); // big endian

	u32 cDescriptor = 0;

	while ((cDescriptor = bReader.read<u32>()) != (u16)-1)
	{
		const u32 cStart = bReader.tell() - 4; // get the offset of the chunk start
		const u32 cLength = bReader.read<u32>();

		if (cStart & 0x1f)
		{
			bReader.warnAt("bReader.tell() isn't aligned with 0x20! ERROR!\n", cStart - 4, cStart);
			return;
		}

		switch (static_cast<Chunks>(cDescriptor))
		{
		case Chunks::Header:
			context.read_header(bReader);
			break;
		case Chunks::VertexPosition:
			readChunk(bReader, context.m_vertices);
			break;
		case Chunks::VertexNormal:
			readChunk(bReader, context.m_vnorms);
			break;
		case Chunks::VertexNBT:
			readChunk(bReader, context.m_nbt);
			break;
		case Chunks::VertexColor:
			readChunk(bReader, context.m_colours);
			break;
		case Chunks::VertexUV0:
		case Chunks::VertexUV1:
		case Chunks::VertexUV2:
		case Chunks::VertexUV3:
		case Chunks::VertexUV4:
		case Chunks::VertexUV5:
		case Chunks::VertexUV6:
		case Chunks::VertexUV7:
			readChunk(bReader, context.m_texcoords[cDescriptor - (int)Chunks::VertexUV0]);
			break;
		case Chunks::Texture:
			readChunk(bReader, context.m_textures);
			break;
		case Chunks::TextureAttribute:
			readChunk(bReader, context.m_texattrs);
			break;
		case Chunks::VertexMatrix:
			readChunk(bReader, context.m_vtxmatrices);
			break;
		case Chunks::Envelope:
			readChunk(bReader, context.m_envelopes);
			break;
		case Chunks::Mesh:
			readChunk(bReader, context.m_batches);
			break;
		case Chunks::Joint:
			context.m_joints.resize(bReader.read<u32>());

			skipPadding(bReader);

			for (auto& joint : context.m_joints) {
				joint << bReader;
				// taken from decomp, if used there good enough to be used here
				context.m_collisionGrid.m_collBounds.expandBound(joint.m_boundingBox);
			}

			skipPadding(bReader);
			break;
		case Chunks::JointName:
			readChunk(bReader, context.m_jointNames);
			break;
		case Chunks::CollisionPrism:
			context.read_basecolltriinfo(bReader);
			break;
		case Chunks::CollisionGrid:
			context.m_collisionGrid << bReader;
			break;
		case Chunks::EoF: // caught because it's not a valid chunk to read, so don't even bother warning user and just break
			break;
		default:
			bReader.warnAt("Unimplemented chunk\n", cStart, cStart + 4);
			skipChunk(bReader, cLength);
			break;
		}
	}

	// Usually, after the EoF chunk, it would be the end of the file
	// But! If there are still bytes after EoF it is assumed there is an INI
	if (bReader.tell() != bReader.endpos())
	{
		DebugReport("INI file found at end of file\n");
	}
	DebugReport("Done reading file\n");
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

}
} // libcube::pikmin1
