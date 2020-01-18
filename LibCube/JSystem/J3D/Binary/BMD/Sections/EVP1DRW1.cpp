#include <LibCube/JSystem/J3D/Binary/BMD/Sections.hpp>

namespace libcube::jsystem {

void readEVP1DRW1(BMDOutputContext& ctx)
{
    auto& reader = ctx.reader;
    // We infer DRW1 -- one bone -> singlebound, else create envelope
	std::vector<DrawMatrix> envelopes;

	// First read our envelope data
	if (enterSection(ctx, 'EVP1'))
	{
		ScopedSection g(reader, "Envelopes");

		u16 size = reader.read<u16>();
		envelopes.resize(size);
		reader.read<u16>();

		const auto [ofsMatrixSize, ofsMatrixIndex, ofsMatrixWeight, ofsMatrixInvBind] = reader.readX<s32, 4>();

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
			// TODO: Fix
			oishii::Jump<oishii::Whence::Set> gInv(reader, g.start + ofsMatrixInvBind + i * 0x30);

			transferMatrix(ctx.mdl.mJoints[i].inverseBindPoseMtx, reader);
		}
	}

	// Now construct vertex draw matrices.
	if (enterSection(ctx, 'DRW1'))
	{
		ScopedSection g(reader, "Vertex Draw Matrix");

		ctx.mdl.mDrawMatrices.clear();
		ctx.mdl.mDrawMatrices.resize(reader.read<u16>());
		reader.read<u16>();

		const auto [ofsPartialWeighting, ofsIndex] = reader.readX<s32, 2>();

		reader.seekSet(g.start);

		int i = 0;
		for (auto& mtx : ctx.mdl.mDrawMatrices)
		{

			const auto multipleInfluences = reader.peekAt<u8>(ofsPartialWeighting + i);
			const auto index = reader.peekAt<u16>(ofsIndex + i * 2);
			if (multipleInfluences)
				printf("multiple influences: %u\n", index);

			mtx = multipleInfluences ? envelopes[index] : DrawMatrix{ std::vector<DrawMatrix::MatrixWeight>{DrawMatrix::MatrixWeight(index, 1.0f)} };
			i++;
		}
	}
}

}
