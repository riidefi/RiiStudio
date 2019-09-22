#include "MOD.hpp"

namespace libcube {

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
	std::printf("Creation date of model file (YYYY/MM/DD): %u/%u/%u\n", m_header.m_year, m_header.m_month, m_header.m_day);
	skipPadding(bReader);
}

inline void MOD::read_vertices(oishii::BinaryReader& bReader)
{
	std::printf("Reading vertices\n");
	m_vertexCount = bReader.read<u32>();
	skipPadding(bReader);
	m_vertices.resize(m_vertexCount);
	for (u32 i = 0; i < m_vertexCount; i++)
	{
		const auto verticesPtr = m_vertices.data();
		MOD_readVec3(bReader, verticesPtr[i]);
		std::printf("X %f Y %f Z %f\n", verticesPtr[i].x, verticesPtr[i].y, verticesPtr[i].z);
	}
	skipPadding(bReader);
}

inline void MOD::read_vnormals(oishii::BinaryReader& bReader)
{
	std::printf("Reading vertex normals\n");
	m_vNormalCount = bReader.read<u32>();
	skipPadding(bReader);
	m_vnorms.resize(m_vNormalCount);
	for (u32 i = 0; i < m_vNormalCount; i++)
	{
		const auto vnormsPtr = m_vnorms.data();
		MOD_readVec3(bReader, vnormsPtr[i]);
		std::printf("X %f Y %f Z %f\n", vnormsPtr[i].x, vnormsPtr[i].y, vnormsPtr[i].z);
	}
	skipPadding(bReader);
}

inline void MOD::skipPadding(oishii::BinaryReader& bReader)
{
	const u32 currentPos = bReader.tell();
	const u32 toRead = (~(0x20 - 1) & (currentPos + 0x20 - 1)) - currentPos;
	skipChunk(bReader, toRead);
}

inline void MOD::skipChunk(oishii::BinaryReader& bReader, u32 offset)
{
	bReader.seek<oishii::Whence::Current>(offset);
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
		default:
			std::printf("Got chunk %04x, not implemented yet!\n", cDescriptor);
			skipChunk(bReader, cLength);
			break;
		}
	} while (cDescriptor != 0xFFFF);
}

}
