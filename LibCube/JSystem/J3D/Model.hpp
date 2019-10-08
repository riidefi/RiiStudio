#pragma once

#include <LibRiiEditor/pluginapi/FileState.hpp>
#include <LibRiiEditor/pluginapi/Interfaces/TextureList.hpp>
#include <LibCube/GX/VertexTypes.hpp>
#include <map>
#include <string>
#include <vector>
#include <ThirdParty/glm/mat4x4.hpp>

#include <LibCube/Common/BoundBox.hpp>

namespace libcube { namespace jsystem {

// Not a FileState for now -- always accessed through a J3DCollection.
// This is subject to change
// No compatibility with old BMD files yet.
struct J3DModel
{
	template<typename T>
	using ID = std::string;

	// TODO
	struct Material;
	struct Shape;

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

		// Hierarchy data is included in joints.
	};

	Information info;

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
			// TODO: Proper reference system
			/* std::string */ u32 boneId;
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

		ID<Joint> id;

		u16 flag; // Unused four bits; default value in galaxy is 1
		MatrixType bbMtxType;
		bool mayaSSC; // 0xFF acts as false -- likely for compatibility

		glm::vec3 scale, rotate, translate;

		f32 boundingSphereRadius;
		AABB boundingBox;

		// From INF1
		ID<Joint> parent;
		std::vector<ID<Joint>> children;

		struct Display
		{
			ID<Material> material;
			ID<Shape> shape;
		};
		std::vector<Display> displays;

		// From EVP1
		glm::mat4x4 inverseBindPoseMtx;
	};

	std::vector<Joint> mJoints;
};

} } // namespace libcube::jsystem
