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
		u32 m_numJoints = 0;
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
