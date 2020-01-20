#include "MOD_IO.hpp"

namespace libcube { namespace pikmin1 {

bool MODImporter::tryRead(oishii::BinaryReader& bReader, pl::FileState& state)
{
	Model& mdl = static_cast<Model&>(state);

	// Chunk descriptor
	u32 cDescriptor = 0;
	// 0xFFFF means EoF for this file, aka (u16)-1
	while (/* bReader.tell() < bReader.endpos() && */(cDescriptor = bReader.read<u32>()) != (u16)-1)
	{
		// Chunk start
		const u32 cStart = bReader.tell() - 4;
		// Chunk length
		const u32 cLength = bReader.read<u32>();

		// Check if the chunk start is aligned to the nearest 32 bytes
		if (cStart & 0x1F)
		{
			bReader.warnAt("Chunk start isn't aligned to 32 bytes! Error!\n", cStart - 4, cStart);
			return false;
		}

		switch (static_cast<Chunks>(cDescriptor))
		{
		case Chunks::Header:
			readHeader(bReader, mdl);
			break;
		case Chunks::VertexPosition:
			readChunk(bReader, mdl.mVertexBuffers.mPosition);
			break;
		case Chunks::VertexNormal:
			readChunk(bReader, mdl.mVertexBuffers.mNormal);
			break;
		case Chunks::VertexNBT:
			readChunk(bReader, mdl.mVertexBuffers.mNBT);
			break;
		case Chunks::VertexColor:
			readChunk(bReader, mdl.mVertexBuffers.mColor);
			break;
		case Chunks::VertexUV0:
		case Chunks::VertexUV1:
		case Chunks::VertexUV2:
		case Chunks::VertexUV3:
		case Chunks::VertexUV4:
		case Chunks::VertexUV5:
		case Chunks::VertexUV6:
		case Chunks::VertexUV7:
			readChunk(bReader, mdl.mVertexBuffers.mTexCoords[cDescriptor - (int)Chunks::VertexUV0]);
			break;
		case Chunks::Texture:
			readChunk(bReader, mdl.mTextures);
			break;
		case Chunks::TextureAttribute:
			readChunk(bReader, mdl.mTexAttrs);
			break;
		case Chunks::VertexMatrix:
			readChunk(bReader, mdl.mVertexMatrices);
			break;
		case Chunks::Envelope:
			readChunk(bReader, mdl.mEnvelopes);
			break;
		case Chunks::Mesh:
			readChunk(bReader, mdl.mMeshes);
			break;
		case Chunks::Joint:
			mdl.mJoints.resize(bReader.read<u32>());

			skipPadding(bReader);

			for (auto& joint : mdl.mJoints) {
				// Read joints
				joint << bReader;
				// Expand the collision grid boundaries based on joints
				mdl.mCollisionModel.m_collisionGrid.m_collBounds.expandBound(joint.m_boundingBox);
			}

			skipPadding(bReader);
			break;
		case Chunks::JointName:
			readChunk(bReader, mdl.mJointNames);
			break;
		case Chunks::CollisionPrism:
			readCollisionPrism(bReader, mdl);
			break;
		case Chunks::CollisionGrid:
			mdl.mCollisionModel.m_collisionGrid << bReader;
			break;
		// Caught because 'EoF' is in every file, therefore it is not an unimplemented chunk
		case Chunks::EoF:
			break;
		default:
			// Only chunk left to do is the Material (0x30) chunk
			bReader.warnAt("Unimplemented chunk\n", cStart, cStart + 4);
			// Because we haven't implemented it, we'll have to skip it
			skipChunk(bReader, cLength);
			break;
		}

		if (bReader.tell() > bReader.endpos())
		{
			printf("Fatal error.. attempted to read beyond end of the stream\n");
			throw "Read beyond stream";
		}
	}

	// Construct joint relationships
	for (int i = 0; i < mdl.mJoints.size(); ++i)
	{
		auto& j = mdl.mJoints[i];

		j.mID = i;
		if (j.m_parentIndex != -1)
			mdl.mJoints[j.m_parentIndex].mChildren.push_back(j.mID);
		if (mdl.mJointNames.empty())
			j.mName = std::string("Joint #") + std::to_string(i);
	}

	// Usually, after the EoF chunk, it would be the end of the file
	// But! If there are still bytes after EoF it is assumed there is an INI
	if (bReader.tell() != bReader.endpos())
	{
		DebugReport("INI file found at end of file\n");
	}
	DebugReport("Done reading file\n");

	// PrintMODStats(context);

	return true;
}

void MODImporter::readHeader(oishii::BinaryReader& bReader, Model& mdl)
{
	skipPadding(bReader);

	mdl.mHeader.year = bReader.read<u16>();
	mdl.mHeader.month = bReader.read<u8>();
	mdl.mHeader.day = bReader.read<u8>();
	DebugReport("Creation date of model file (YYYY/MM/DD): %u/%u/%u\n", mdl.mHeader.year, mdl.mHeader.month, mdl.mHeader.day);
	// Used to identify what kind of system is being used,
	// Can be used to identify if using NBT or Classic/Softimage scaling system
	mdl.mHeader.systemsFlag = bReader.read<u32>();
	const bool usingEmboss = mdl.mHeader.systemsFlag & 1;
	const bool whichScaling = mdl.mHeader.systemsFlag & 8;
	if (usingEmboss)
		DebugReport("Model file using Emboss NBT!\n");
	if (whichScaling)
		DebugReport("Model file using Classic Scaling System!\n");
	else
		DebugReport("Model file using SoftImage Scaling System!\n");

	skipPadding(bReader);
}

void MODImporter::readCollisionPrism(oishii::BinaryReader& bReader, Model& mdl)
{
	DebugReport("Reading collision triangle information\n");
	mdl.mCollisionModel.m_baseCollTriInfo.resize(bReader.read<u32>());
	mdl.mCollisionModel.m_baseRoomInfo.resize(bReader.read<u32>());

	skipPadding(bReader);
	for (auto& info : mdl.mCollisionModel.m_baseRoomInfo)
		info << bReader;

	skipPadding(bReader);
	for (auto& collTri : mdl.mCollisionModel.m_baseCollTriInfo)
		collTri << bReader;

	skipPadding(bReader);
}

void Model::removeMtxDependancy()
{
	for (auto& batch : mMeshes)
	{
		batch.m_depMTXGroups = 0;
		for (auto& mtx_group : batch.m_mtxGroups)
		{
			mtx_group.m_dependant.clear();
		}
	}
}

} } // namespace libcube::pikmin1
