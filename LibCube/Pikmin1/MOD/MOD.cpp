#include "MOD.hpp"
#include <fstream>

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

static std::vector<glm::ivec3> ToTris(std::vector<u32>& polys, u32 opcode)
{
	std::vector<glm::ivec3> returnVal;

	if (polys.size() == 3)
	{
		returnVal.push_back(glm::ivec3(polys[0], polys[1], polys[2]));

		return returnVal;
	}

	if (opcode == 0x98)
	{
		u32 n = 2;
		for (u32 i = 0; i < polys.size() - 2; i++)
		{
			u32 tri[3];
			const bool isEven = (n % 2) == 0;
			tri[0] = polys[n - 2];
			tri[1] = isEven ? polys[n] : polys[n - 1];
			tri[2] = isEven ? polys[n - 1] : polys[n];

			if (tri[0] != tri[1] && tri[1] != tri[2] && tri[2] != tri[0])
			{
				returnVal.push_back(glm::ivec3(tri[0], tri[1], tri[2]));
			}

			n++;
		}
	}
	if (opcode == 0x98)
	{
		for (u32 n = 1; n < polys.size() - 1; n++)
		{
			u32 tri[3];
			tri[0] = polys[n];
			tri[1] = polys[n + 1];
			tri[2] = polys[0];

			if (tri[0] != tri[1] && tri[1] != tri[2] && tri[2] != tri[0])
			{
				returnVal.push_back(glm::ivec3(tri[0], tri[1], tri[2]));
			}
		}
	}

	return returnVal;
}

static void OutputFaces(std::ofstream& objFile, MOD& toPrint)
{
	for (u32 i = 0; i < toPrint.m_batches.size(); i++)
	{
		objFile << "o Mesh" << i << '\n';
		const u32 vcd = toPrint.m_batches[i].m_vcd.m_originalVCD;
		for (const auto& mtxGroup : toPrint.m_batches[i].m_mtxGroups)
		{
			for (const auto& dispList : mtxGroup.m_dispLists)
			{
				oishii::BinaryReader bReader(dispList.m_dispData, dispList.m_dispData.size());
				bReader.setEndian(true);
				bReader.seekSet(0);

				const u8 faceOpcode = bReader.read<u8>();
				if (faceOpcode == 0x98 || faceOpcode == 0xA0)
				{
					const u16 vCount = bReader.read<u16>();

					std::vector<u32> polys(vCount);
					u32 currentPolys = 0;
					for (u32 vC = 0; vC < vCount; vC++)
					{
						if ((vcd & 0x1) == 0x1)
							bReader.read<u8>(); // position matrix
						if ((vcd & 0x2) == 0x2)
							bReader.read<u8>(); // tex1 matrix
						bReader.read<u16>(); // vertex position index

						if (toPrint.m_vnorms.size() > 0)
							bReader.read<u16>(); // vertex normal index
						if ((vcd & 0x4) == 0x4)
							bReader.read<u16>(); // vertex colour index

						int tmpvar = vcd >> 3;
						for (u32 j = 0; j < 8; j++)
						{
							if ((tmpvar & 0x1) == 0x1)
								if (j == 0) bReader.read<u16>();
							tmpvar >>= 1;
						}

						polys[vC] = currentPolys;
						currentPolys++;
					}

					std::vector<glm::ivec3> toOutput = ToTris(polys, faceOpcode);
					for (const auto& elem : toOutput)
					{
						objFile << "f " << elem.x << ' ' << elem.y << ' ' << elem.z << '\n';
					}
				}
			}
		}
	}
}

static void PrintMODStats(MOD& toPrint)
{
	DebugReport("There are %zu %s\n", toPrint.m_vertices.size(), toPrint.m_vertices.name);
	DebugReport("There are %zu %s\n", toPrint.m_vnorms.size(), toPrint.m_vnorms.name);
	DebugReport("There are %zu %s\n", toPrint.m_nbt.size(), toPrint.m_nbt.name);
	DebugReport("There are %zu %s\n", toPrint.m_colours.size(), toPrint.m_colours.name);
	for (u32 i = 0; i < 8; i++)
		DebugReport("There are %zu Texture Coordinates in TEXCOORD%d\n", toPrint.m_texcoords[i].size(), i);
	DebugReport("There are %zu %s\n", toPrint.m_textures.size(), toPrint.m_textures.name);
	DebugReport("There are %zu %s\n", toPrint.m_texattrs.size(), toPrint.m_texattrs.name);
	DebugReport("There are %zu %s\n", toPrint.m_vtxmatrices.size(), toPrint.m_vtxmatrices.name);
	DebugReport("There are %zu %s\n", toPrint.m_envelopes.size(), toPrint.m_envelopes.name);
	DebugReport("There are %zu %s\n", toPrint.m_batches.size(), toPrint.m_batches.name);
	DebugReport("There are %zu %s\n", toPrint.m_joints.size(), toPrint.m_joints.name);
	DebugReport("There are %zu %s\n", toPrint.m_jointNames.size(), toPrint.m_jointNames.name);
	DebugReport("There are %zu groups of basic collision triangle information\n", toPrint.m_baseCollTriInfo.size());
	DebugReport("There are %zu groups of basic room information\n", toPrint.m_baseRoomInfo.size());

	std::ofstream objFile("test.obj");
	objFile << "# Made with RiiStudio, by Riidefi & Ambrosia\n";

	for (const auto& vertex : toPrint.m_vertices)
		objFile << "v " << vertex.x << ' ' << vertex.y << ' ' << vertex.z << '\n';

	for (u32 i = 0; i < 8; i++)
	{
		for (const auto& UV : toPrint.m_texcoords[i])
			objFile << "vt " << UV.x << ' ' << UV.y << '\n';
	}

	for (const auto& vertex : toPrint.m_vnorms)
		objFile << "vn " << vertex.x << ' ' << vertex.y << ' ' << vertex.z << '\n';

	if (toPrint.m_baseCollTriInfo.size() != 0)
	{
		objFile << "o CollisionMesh\n";
		for (u32 i = 0; i < toPrint.m_baseCollTriInfo.size(); i++)
		{
			objFile << "f " << toPrint.m_baseCollTriInfo[i].m_faceA+1 << ' ' << toPrint.m_baseCollTriInfo[i].m_faceC+1 << ' ' << toPrint.m_baseCollTriInfo[i].m_faceB+1 << '\n';
		}
	}
	else
	{
		if (toPrint.m_batches.size())
			OutputFaces(objFile, toPrint);
	}

	objFile.close();
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

	PrintMODStats(context);
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
