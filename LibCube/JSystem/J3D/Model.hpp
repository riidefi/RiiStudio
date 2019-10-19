#pragma once

#include <LibRiiEditor/pluginapi/FileState.hpp>
#include <LibRiiEditor/pluginapi/Interfaces/TextureList.hpp>
#include <LibCube/GX/VertexTypes.hpp>
#include <map>
#include <string>
#include <vector>
#include <ThirdParty/glm/mat4x4.hpp>

#include <LibCube/Common/BoundBox.hpp>

#include <LibCube/GX/Material.hpp>


namespace libcube { namespace jsystem {

// Not a FileState for now -- always accessed through a J3DCollection.
// This is subject to change
// No compatibility with old BMD files yet.
struct J3DModel
{
	template<typename T>
	using ID = std::string;

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

			MatrixWeight()
			: boneId(-1), weight(0.0f)
			{}
			MatrixWeight(u32 b, f32 w)
			: boneId(b), weight(w)
			{}
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

			Display() = default;
			Display(ID<Material> mat, ID<Shape> shp)
				: material(mat), shape(shp)
			{}
		};
		std::vector<Display> displays;

		// From EVP1
		glm::mat4x4 inverseBindPoseMtx;
	};

	std::vector<Joint> mJoints;

	using todo = int;

	// Assumption: all elements are contiguous--no holes
	// Much faster than a vector for the many static sized arrays in materials
	template<typename T, size_t N>
	struct array_vector : public std::array<T, N>
	{
		size_t size() const
		{
			return nElements;
		}
		size_t nElements = 0;

		void push_back(T elem)
		{
			at(nElements) = elem;
		}
		void pop_back()
		{
			--nElements;
		}
	};

	struct Material
	{
		struct GenInfo
		{
			u8 nColorChannel, nTexGen, nTevStage;
			bool indirect;
			u8 nInd;
		};
		struct TexMatrix
		{
			gx::TexGenType projection; // Only 3x4 and 2x4 valid

			bool		maya;
			u8			mappingMethod;

			glm::vec3	origin;

			glm::vec2	scale;
			f32			rotate;
			glm::vec2	translate;

			glm::vec4	effectMatrix;
		};
		struct NBTScale
		{
			bool enable;
			glm::vec3 scale;
		};
		
		ID<Material> id;

		u8 flag;

		GenInfo info;
		gx::CullMode cullMode;

		bool earlyZComparison;
		gx::ZMode ZMode;

		array_vector<gx::Color, 2> matColors;
		array_vector<gx::ChannelControl, 4> colorChanControls;
		array_vector<gx::Color, 2> ambColors;
		array_vector<gx::Color, 8> lightColors;

		array_vector<gx::TexCoordGen, 8> texGenInfos;

		array_vector<TexMatrix, 10> texMatrices;
		array_vector<TexMatrix, 20> postTexMatrices;

		// FIXME: Sampler data will be moved here from TEX1
		array_vector<todo, 8> textures;

		array_vector<gx::Color, 4> tevKonstColors;
		array_vector<gx::ColorS10, 4> tevColors;

		gx::Shader shader;

		// FIXME: This will exist in our scene and be referenced.
		todo fogInfo;

		gx::AlphaComparison alphaCompare;
		gx::BlendMode blendMode;
		bool dither;
		NBTScale nbtScale;
	};
	std::vector<Material> mMaterials;
};

} } // namespace libcube::jsystem
