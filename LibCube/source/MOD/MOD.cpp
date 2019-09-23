#include "MOD.hpp"
#include <common.hpp>

namespace libcube {

namespace pikmin1 {

inline void MOD_readVec3(oishii::BinaryReader& bReader, glm::vec3& vector3)
{
	vector3.x = bReader.read<float>();
	vector3.y = bReader.read<float>();
	vector3.z = bReader.read<float>();
}

inline void MOD::read_header(oishii::BinaryReader& bReader)
{
	skipPadding(bReader);
	m_header.m_year = bReader.read<u16>();
	m_header.m_month = bReader.read<u8>();
	m_header.m_day = bReader.read<u8>();
	m_header.m_unk = bReader.read<u32>();
	DebugReport("Creation date of model file (YYYY/MM/DD): %u/%u/%u\n", m_header.m_year, m_header.m_month, m_header.m_day);
	skipPadding(bReader);
}

inline void MOD::read_vertices(oishii::BinaryReader& bReader)
{
	DebugReport("Reading vertices\n");
	m_vertexCount = bReader.read<u32>();
	skipPadding(bReader);
	m_vertices.resize(m_vertexCount);
	for (u32 i = 0; i < m_vertexCount; i++)
	{
		const auto verticesPtr = m_vertices.data();
		MOD_readVec3(bReader, verticesPtr[i]);
		//DebugReport("X %f Y %f Z %f\n", verticesPtr[i].x, verticesPtr[i].y, verticesPtr[i].z);
	}
	skipPadding(bReader);
}

inline void MOD::read_vnormals(oishii::BinaryReader& bReader)
{
	DebugReport("Reading vertex normals\n");
	m_vNormalCount = bReader.read<u32>();
	skipPadding(bReader);
	m_vnorms.resize(m_vNormalCount);
	for (u32 i = 0; i < m_vNormalCount; i++)
	{
		const auto vnormsPtr = m_vnorms.data();
		MOD_readVec3(bReader, vnormsPtr[i]);
		//DebugReport("X %f Y %f Z %f\n", vnormsPtr[i].x, vnormsPtr[i].y, vnormsPtr[i].z);
	}
	skipPadding(bReader);
}

inline void MOD::read_colours(oishii::BinaryReader& bReader)
{
	DebugReport("Reading mesh colours\n");
	m_colourCount = bReader.read<u32>();
	skipPadding(bReader);
	m_colours.resize(m_colourCount);
	for (u32 i = 0; i < m_colourCount; i++)
	{
		auto coloursPtr = m_colours.data();
		coloursPtr[i].read(bReader);
		DebugReport("Colour %u, R%u G%u B%u A%u\n", i, coloursPtr[i].m_R, coloursPtr[i].m_G, coloursPtr[i].m_B, coloursPtr[i].m_A);
	}
	skipPadding(bReader);
}

inline void MOD::read_faces(oishii::BinaryReader& bReader)
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

inline void MOD::read_jointnames(oishii::BinaryReader & bReader)
{
	DebugReport("Reading joint names\n");
	m_jointNameCount = bReader.read<u32>();
	skipPadding(bReader);
	m_jointNames.resize(m_jointNameCount);
	for (u32 i = 0; i < m_jointNameCount; i++)
	{
		auto jointNamesPtr = m_jointNames.data();
		const u32 nameLength = bReader.read<u32>();

		std::string nameString(nameLength, 0);
		for (u32 j = 0; j < nameLength; ++j)
			nameString[j] = bReader.read<s8>();
		jointNamesPtr[i] = nameString;

		DebugReport("Got joint name %s, string length %u\n", jointNamesPtr[i].c_str(), jointNamesPtr[i].length());
	}
	skipPadding(bReader);
}

void MOD::read(oishii::BinaryReader & bReader)
{
	u32 cDescriptor = 0;
	bReader.setEndian(true);	// big endian
	bReader.seekSet(0);		// reset file pointer position

	do
	{
		const u32 cPosition = bReader.tell();
		cDescriptor = bReader.read<u32>();
		const u32 cLength = bReader.read<u32>();

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
		default:
			DebugReport("Got chunk %04x, not implemented yet!\n", cDescriptor);
			skipChunk(bReader, cLength);
			break;
		}
	} while (cDescriptor != 0xFFFF);
}

} // pikmin1

} // libcube
