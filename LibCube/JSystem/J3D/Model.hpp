#pragma once

#include <LibRiiEditor/pluginapi/FileState.hpp>
#include <LibRiiEditor/pluginapi/Interfaces/TextureList.hpp>
#include <LibCube/GX/VertexTypes.hpp>
#include <map>

namespace libcube { namespace jsystem {

// Not a FileState for now -- always accessed through a J3DCollection.
// This is subject to change
// No compatibility with old BMD files yet.
struct J3DModel
{
	struct Information
	{
		// For texmatrix calculations
		enum class ScalingRule
		{
			Basic,
			XSI,
			Maya
		};

		ScalingRule mScalingRule;
		// nPacket, nVtx
	};

	struct VertexBuffer
	{
		struct Quantization
		{
			gx::VertexComponentCount mComp;
			gx::VertexBufferType mType;
			u8 divisor;
			// TODO: Doesn't appear to store stride
		};
	};

	// TODO -- Is there any reason to keep split in parsed model?
	struct Envelope
	{
		struct Weight
		{
			u8 index;
			f32 weight;
		};
		std::vector<Weight> mWeights;
	};
	std::vector<Envelope> mEnvelopes;

	struct DrawMatrix
	{
		bool multipleInfluences;
		int idx; // if mult, envelope; else, bone
	};
};

} } // namespace libcube::jsystem
