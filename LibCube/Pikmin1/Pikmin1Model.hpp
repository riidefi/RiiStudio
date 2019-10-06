/*!
 * @file
 * @brief FIXME: Document
 */

#pragma once

#include <LibRiiEditor/common.hpp>
#include <vector>
#include <ThirdParty/glm/vec3.hpp>

#include "allincludes.hpp"
#include <LibCube/Pikmin1/TXE/TXE.hpp>
#include <array>

#include <LibRiiEditor/pluginapi/FileState.hpp>
#include <LibRiiEditor/pluginapi/Interfaces/TextureList.hpp>

namespace libcube { namespace pikmin1 {

enum class Chunks : u16
{
	Header = 0x0000,

	VertexPosition = 0x0010,
	VertexNormal = 0x0011,
	VertexNBT = 0x0012,
	VertexColor = 0x0013,

	VertexUV0 = 0x0018,
	VertexUV1 = 0x0019,
	VertexUV2 = 0x001A,
	VertexUV3 = 0x001B,
	VertexUV4 = 0x001C,
	VertexUV5 = 0x001D,
	VertexUV6 = 0x001E,
	VertexUV7 = 0x001F,

	Texture = 0x0020,
	TextureAttribute = 0x0022,
	Material = 0x0030,

	VertexMatrix = 0x0040,

	Envelope = 0x0041,

	Mesh = 0x0050,

	Joint = 0x0060,
	JointName = 0x0061,

	CollisionPrism = 0x0100,
	CollisionGrid = 0x0110,

	EoF = 0xFFFF
};

//! FIXME: Document
//!
class Model : public pl::FileState, pl::ITextureList
{
public:
	//! FIXME: Document
	//
	struct Header
	{
		u16 year;
		u8 month;
		u8 day;

		u32 systemsFlag; //!< TODO -- Define flag enum
	};

	Header mHeader;

	struct VertexAttributeBuffers
	{
		std::vector<glm::vec3> mPosition;
		std::vector<glm::vec3> mNormal;
		std::vector<NBT>	   mNBT;
		std::vector<Colour>	   mColor;
		std::array<std::vector<glm::vec2>, 8> mTexCoords;
	};

	VertexAttributeBuffers	mVertexBuffers;
	std::vector<VtxMatrix>	mVertexMatrices;
	std::vector<Batch>		mMeshes;
	std::vector<Joint>		mJoints;
	std::vector<String>		mJointNames;
	std::vector<TXE>		mTextures;
	std::vector<TexAttr>	mTexAttrs;
	std::vector<Material>	mMaterials;
	std::vector<PVWTevInfo> mShaders;
	std::vector<Envelope>	mEnvelopes;

	struct CollisionModel
	{
		std::vector<BaseCollTriInfo> m_baseCollTriInfo;
		std::vector<BaseRoomInfo> m_baseRoomInfo;
		CollGroup m_collisionGrid;
	};

	CollisionModel mCollisionModel;


	//
	// Interfaces
	//

	// ITextureList
	u32 getNumTex() const override
	{
		return mTextures.size();
	}
	std::string getNameAt(int idx) const override
	{
		return "TODO";
	}

	//
	// Construction/Destruction
	//

	~Model() override = default;
	Model()
	{
		// Register interfaces
		mInterfaces.push_back(static_cast<pl::ITextureList*>(this));
	}
};

} } // namespace libcube::pikmin1
