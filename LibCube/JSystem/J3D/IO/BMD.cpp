/*!
 * @file
 * @brief FIXME: Document
 */

#pragma once

#include "BMD.hpp"
#include "../Model.hpp"
#include <map>

namespace libcube { namespace jsystem {

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

	static void onRead(oishii::BinaryReader& reader, BMDImporter::BMDOutputContext& ctx)
	{
		for (ByteCodeCmd cmd(reader); cmd.op != ByteCodeOp::End; cmd.transfer(reader))
		{
			// TODO
		}
	}
};

bool BMDImporter::enterSection(oishii::BinaryReader& reader, u32 id)
{
	const auto sec = mSections.find(id);
	if (sec == mSections.end())
		return false;

	reader.seekSet(sec->second.streamPos - 8);
	return true;
}

struct ScopedSection : private oishii::BinaryReader::ScopedRegion
{
	ScopedSection(oishii::BinaryReader& reader, const char* name)
		: oishii::BinaryReader::ScopedRegion(reader, name)
	{
		start = reader.tell();
		reader.seek(8);
	}
	u32 start;
};

void BMDImporter::readInformation(oishii::BinaryReader& reader, BMDOutputContext& ctx) noexcept
{
	if (enterSection(reader, 'INF1'))
	{
		ScopedSection g(reader, "Information");

		u32 flag = reader.read<u32>();

		// TODO -- Use these for validation
		u32 nPacket = reader.read<u32>();
		u32 nVertex = reader.read<u32>();

		ctx.mdl.info.mScalingRule = static_cast<J3DModel::Information::ScalingRule>(flag & 0xf);

		reader.dispatch<SceneGraph, oishii::Indirection<-8, s32>>(ctx);
	}
}
void BMDImporter::readDrawMatrices(oishii::BinaryReader& reader, BMDOutputContext& ctx) noexcept
{

	// We infer DRW1 -- one bone -> singlebound, else create envelope
	std::vector<J3DModel::DrawMatrix> envelopes;

	// First read our envelope data
	if (enterSection(reader, 'EVP1'))
	{
		ScopedSection g(reader, "Envelopes");

		u16 size = reader.read<u16>();
		envelopes.resize(size);
		reader.read<u16>();

		const auto[ofsMatrixSize, ofsMatrixIndex, ofsMatrixWeight, ofsMatrixInvBind] = reader.readX<s32, 4>();

		int mtxId = 0;
		int maxJointIndex = -1;

		reader.seekSet(g.start);

		for (int i = 0; i < size; ++i)
		{
			const auto num = reader.peekAt<u8>(ofsMatrixSize + i);

			for (int j = 0; j < num; ++j)
			{
				const auto index = reader.peekAt<u16>(ofsMatrixIndex + mtxId * 2);
				const auto influence = reader.peekAt<f32>(ofsMatrixWeight + mtxId * 4);

				envelopes[i].mWeights.emplace_back(index, influence);

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
	if (enterSection(reader, 'DRW1'))
	{
		ScopedSection g(reader, "Vertex Draw Matrix");

		ctx.mdl.mDrawMatrices.clear();
		ctx.mdl.mDrawMatrices.resize(reader.read<u16>());
		reader.read<u16>();

		const auto[ofsPartialWeighting, ofsIndex] = reader.readX<s32, 2>();

		reader.seekSet(g.start);

		int i = 0;
		for (auto& mtx : ctx.mdl.mDrawMatrices)
		{
			ScopedInc inc(i);

			const auto multipleInfluences = reader.peekAt<u8>(ofsPartialWeighting + i);
			const auto index = reader.peekAt<u16>(ofsIndex + i * 2);

			mtx = multipleInfluences ? envelopes[index] : J3DModel::DrawMatrix{ std::vector<J3DModel::DrawMatrix::MatrixWeight>{index} };
		}
	}
}

void BMDImporter::lex(oishii::BinaryReader& reader, u32 sec_count) noexcept
{
	mSections.clear();
	for (u32 i = 0; i < sec_count; ++i)
	{
		const u32 secType = reader.read<u32>();
		const u32 secSize = reader.read<u32>();

		{
			oishii::JumpOut g(reader, secSize - 8);
			switch (secType)
			{
			case 'INF1':
			case 'VTX1':
			case 'EVP1':
			case 'DRW1':
			case 'JNT1':
			case 'SHP1':
			case 'MAT3':
			case 'MDL3':
			case 'TEX1':
				mSections[secType] = { reader.tell(), secSize };
				break;
			default:
				reader.warnAt("Unexpected section type.", reader.tell() - 8, reader.tell() - 4);
				break;
			}
		}
	}
}
void BMDImporter::readBMD(oishii::BinaryReader& reader, BMDOutputContext& ctx)
{
	reader.expectMagic<'J3D2'>();


	u32 bmdVer = reader.read<u32>();
	if (bmdVer != 'bmd3' && bmdVer != 'bdl4')
	{
		reader.signalInvalidityLast<u32, oishii::MagicInvalidity<'bmd3'>>();
		error = true;
		return;
	}

	// TODO: Validate file size.
	const auto fileSize = reader.read<u32>();
	const auto sec_count = reader.read<u32>();

	// Skip SVR3 header
	reader.seek<oishii::Whence::Current>(16);

	// Skim through sections
	lex(reader, sec_count);

	// Read VTX1
	// Read EVP1 and DRW1
	readDrawMatrices(reader, ctx);
	// Read JNT1

	// Read SHP1

	// Read TEX1
	// Read MAT3

	// Read INF1
	readInformation(reader, ctx);

	// FIXME: MDL3
}

bool BMDImporter::tryRead(oishii::BinaryReader& reader, pl::FileState& state)
{
	//oishii::BinaryReader::ScopedRegion g(reader, "J3D Binary Model Data (.bmd)");
	readBMD(reader, BMDOutputContext{ static_cast<J3DCollection&>(state).mModel });
	return !error;
}

} } // namespace libcube::jsystem
