/*!
 * @file
 * @brief FIXME: Document
 */

#pragma once

#include "BMD.hpp"
#include "../Model.hpp"
#include <map>

namespace libcube { namespace jsystem {

struct J3DModelLoadingContext
{
	J3DModel& mdl;
	bool error = false;

	
	// Associate section magics with file positions and size
	struct SectionEntry
	{
		std::size_t streamPos;
		u32 size;
	};
	std::map<u32, SectionEntry> mSections;

	void lex(oishii::BinaryReader& reader, u32 sec_count) noexcept
	{
		mSections.clear();
		for (u32 i = 0; i < sec_count; ++i)
		{
			const u32 secType = reader.read<u32>();
			const u32 secSize = reader.read<u32>();

			oishii::JumpOut g(reader, secSize - 8);
			{
				switch (secType)
				{
				default:
				//case 'INF1':

					mSections[secType] = { reader.tell(), secSize };
					break;

				//	default:
				//		reader.warnAt("Unexpected section type.", reader.tell() - 8, reader.tell() - 4);
				//		break;
				}
			}
		}
	}

	void readDrawMatrices(oishii::BinaryReader& reader) noexcept
	{

		// We infer DRW1 -- one bone -> singlebound, else create envelope
		std::vector<J3DModel::DrawMatrix> envelopes;

		// First read our envelope data
		const auto evp1 = mSections.find('EVP1');
		if (evp1 != mSections.end())
		{
			const auto start = evp1->second.streamPos - 8;
			reader.seekSet(start + 8);

			u16 size = reader.read<u16>();
			envelopes.resize(size);
			reader.read<u16>();

			// TODO: clean this up
			const auto ofsMatrixSize = start + reader.read<s32>();
			const auto ofsMatrixIndex = start + reader.read<s32>();
			const auto ofsMatrixWeight = start + reader.read<s32>();
			const auto ofsMatrixInvBind = start + reader.read<s32>();

			int mtxId = 0;
			int maxJointIndex = -1;

			for (int i = 0; i < size; ++i)
			{
				// TODO: clean this up
				const auto num = reader.peekAt<u8>(-reader.tell() + ofsMatrixSize + i);

				for (int j = 0; j < num; ++j)
				{
					const auto index = reader.peekAt<u16>(-reader.tell() + ofsMatrixIndex + mtxId * 2);
					const auto influence = reader.peekAt<f32>(-reader.tell() + ofsMatrixWeight + mtxId *4);

					envelopes[i].mWeights.push_back(J3DModel::DrawMatrix::MatrixWeight{index, influence});

					if (index > maxJointIndex)
						maxJointIndex = index;

					++mtxId;
				}
			}

			for (int i = 0; i < maxJointIndex + 1; ++i)
			{
				// TODO: Read inverse bind matrices
			}
		}

		// Now construct vertex draw matrices.
		const auto drw1 = mSections.find('DRW1');
		if (drw1 != mSections.end())
		{
			const auto start = evp1->second.streamPos - 8;
			reader.seekSet(start + 8);

			u16 size = reader.read<u16>();
			mdl.mDrawMatrices.clear();
			mdl.mDrawMatrices.reserve(size);
			reader.read<u16>();

			const auto ofsPartialWeighting = start + reader.read<s32>();
			const auto ofsIndex = start + reader.read<s32>();

			for (int i = 0; i < size; ++i)
			{
				bool multipleInfluences = reader.peekAt<bool>(-reader.tell() + ofsPartialWeighting + i);
				u16 index = reader.peekAt<u16>(-reader.tell() + ofsIndex + i * 2);

				if (!multipleInfluences)
					mdl.mDrawMatrices.push_back(J3DModel::DrawMatrix{std::vector<J3DModel::DrawMatrix::MatrixWeight>{index}});
				else
					mdl.mDrawMatrices.push_back(envelopes[index]);
			}
		}
	}
};

struct INF1Handler
{
	struct SceneGraph
	{
		static constexpr const char name[] = "Scenegraph";

		enum class ByteCodeOp : u16
		{
			End,
			Open,
			Close,

			Joint = 0x10,
			Material,
			Shape,

			Unitialized = -1
		};

		static_assert(sizeof(ByteCodeOp) == 2, "Invalid enum size.");

		struct ByteCodeCmd
		{
			ByteCodeOp op;
			s16 idx;

			ByteCodeCmd(oishii::BinaryReader& reader)
			{
				transfer(reader);
			}
			ByteCodeCmd()
				: op(ByteCodeOp::Unitialized), idx(-1)
			{}

			template<typename T>
			void transfer(T& stream)
			{
				stream.transfer<ByteCodeOp>(op);
				stream.transfer<s16>(idx);
			}
			
		};

		static void onRead(oishii::BinaryReader& reader, J3DModelLoadingContext& ctx)
		{
			for (ByteCodeCmd cmd(reader); cmd.op != ByteCodeOp::End; cmd.transfer(reader))
			{
				// TODO
			}
		}
	};


	static constexpr const char name[] = "Hierarchy/Display Information (INF1)";

	// Call after joints, materials, and shapes have been loaded
	static void onRead(oishii::BinaryReader& reader, J3DModelLoadingContext& ctx)
	{
		u32 flag = reader.read<u32>();

		// TODO -- Use these for validation
		u32 nPacket = reader.read<u32>();
		u32 nVertex = reader.read<u32>();

		ctx.mdl.info.mScalingRule = static_cast<J3DModel::Information::ScalingRule>(flag & 0xf);

		reader.dispatch<SceneGraph, oishii::Indirection<-8, s32>>(ctx);
	}
};


struct BMDHandler
{
	static constexpr const char name[] = "JSystem J3D Binary Model Data (.bmd)";

	static void onRead(oishii::BinaryReader& reader, J3DModelLoadingContext& ctx)
	{
		reader.expectMagic<'J3D2'>();


		u32 bmdVer = reader.read<u32>();
		if (bmdVer != 'bmd3' && bmdVer != 'bdl4')
		{
			reader.signalInvalidityLast<u32, oishii::MagicInvalidity<'bmd3'>>();
			ctx.error = true;
			return;
		}

		// TODO: Validate file size.
		const auto fileSize= reader.read<u32>();
		const auto sec_count = reader.read<u32>();

		// Skip SVR3 header
		reader.seek<oishii::Whence::Current>(16);

		// Skim through sections
		ctx.lex(reader, sec_count);

		// Read VTX1
		// Read EVP1 and DRW1
		ctx.readDrawMatrices(reader);
		// Read JNT1
		
		// Read SHP1

		// Read TEX1
		// Read MAT3

		// Read INF1

		// FIXME: MDL3
	}
};


bool BMDImporter::tryRead(oishii::BinaryReader& reader, pl::FileState& state)
{
	J3DCollection& collection = static_cast<J3DCollection&>(state);
	collection.mModels.emplace_back(new J3DModel());

	J3DModelLoadingContext BMDLoad = {
		*(collection.mModels[collection.mModels.size()-1].get()),
		false
	};

	reader.dispatch<BMDHandler, oishii::Direct, false>(BMDLoad);
	
	return !BMDLoad.error;
}

} } // namespace libcube::jsystem
