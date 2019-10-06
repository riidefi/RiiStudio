#pragma once

#include <LibRiiEditor/pluginapi/FileState.hpp>
#include <LibRiiEditor/pluginapi/Interfaces/TextureList.hpp>
#include <LibCube/GX/VertexTypes.hpp>
#include <map>
#include <string>
#include <vector>

#include <LibCube/Common/BoundBox.hpp>

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

	//! Encapsulates low level envelopes and draw matrices
	struct DrawMatrix
	{
		struct MatrixWeight
		{
			std::string boneId;
			f32 weight;
		};

		std::vector<MatrixWeight> mWeights; // 1 weight -> singlebound, no envelope
	};

	std::vector<DrawMatrix> mDrawMatrices;

	struct Joint
	{
		// Four LSBs of flag; left is matrix type
		enum class MatrixType : u16
		{
			Standard = 0,
			Billboard,
			BillboardY
		};

		u16 flag; // Unused four bits, default value in galaxy is 1
		MatrixType bbMtxType;
		bool mayaSSC; // 0xFF acts as false -- likely compat

		glm::vec3 scale, rotate, translate;

		f32 boundingSphereRadius;
		AABB boudingBox;
	};

	std::vector<Joint> mJoints;
};

} } // namespace libcube::jsystem
