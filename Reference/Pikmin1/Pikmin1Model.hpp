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
#include <LibRiiEditor/pluginapi/FileStateSpawner.hpp>

#include <LibCube/Export/GCCollection.hpp>

namespace libcube { namespace pikmin1 {

using libcube::operator<<;

//! FIXME: Document
//!
class Model : public pl::FileState,
	public GCCollection,
	pl::ITextureList
{
public:
	template<typename T, const char N[]>
	struct Holder : public std::vector<T>
	{
		static constexpr const char* name = N;

		static void onRead(oishii::BinaryReader& reader, Holder& out)
		{
			out.resize(reader.read<u32>());

			skipPadding(reader);

			for (auto& it : out)
				it << reader;

			skipPadding(reader);
		}
	};

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
	struct Names
	{
		static constexpr char positions[] = "Positions";
		static constexpr char normals[] = "Normals";
		static constexpr char nbt[] = "NBT";
		static constexpr char colours[] = "Colors";
		static constexpr char vtxmatrices[] = "Vertex Matrices";
		static constexpr char batches[] = "Batches";
		static constexpr char joints[] = "Joints";
		static constexpr char jointnames[] = "Joint Names";
		static constexpr char textures[] = "Textures";
		static constexpr char texcoords[] = "Texture Coordinates";
		static constexpr char texattribs[] = "Texture Attributes";
		static constexpr char materials[] = "Materials";
		static constexpr char tevinfos[] = "Texture Environments";
		static constexpr char envelope[] = "Skinning Envelope";
	};
	struct VertexAttributeBuffers
	{
		Holder<glm::vec3,	Names::positions>	mPosition;
		Holder<glm::vec3,	Names::normals>		mNormal;
		Holder<NBT,			Names::nbt>			mNBT;
		Holder<Colour,		Names::colours>		mColor;
		std::array<Holder<glm::vec2, Names::texcoords>, 8> mTexCoords;
	};

	VertexAttributeBuffers	mVertexBuffers;

	Holder<VtxMatrix,	Names::vtxmatrices>	mVertexMatrices;
	Holder<Batch,		Names::batches>		mMeshes;
	Holder<Joint,		Names::joints>		mJoints;
	Holder<String,		Names::jointnames>	mJointNames;
	Holder<TXE,			Names::textures>	mTextures;
	Holder<TexAttr,		Names::texattribs>	mTexAttrs;
	Holder<Material,	Names::materials>	mMaterials;
	Holder<PVWTevInfo,	Names::tevinfos>	mShaders;
	Holder<Envelope,	Names::envelope>	mEnvelopes;

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
	
	// pl::CollectionBinder<std::vector<Joint>, BoneCollection> mBoneView{ mJoints };
	// ITextureList
	u64 getNumTex() const override
	{
		return mTextures.size();
	}
	std::string getNameAt(s64 idx) const override
	{
		return "TODO";
	}

	// GC Collection
	
	//
	// Construction/Destruction
	//

	~Model() override = default;
	Model() = default;


	void removeMtxDependancy();

};


class MODSpawner : public pl::FileStateSpawner
{
public:
	~MODSpawner() override = default;
	MODSpawner()
	{
		mId = { "Pikmin 1 Model", "pikmin1mod", "pikmin1mod" };
		addMirror("pikmin1mod", "gc_collection", pl::computeTranslation<Model, GCCollection>());
	}

	std::unique_ptr<pl::FileState> spawn() const override
	{
		return std::make_unique<Model>();
	}
	std::unique_ptr<FileStateSpawner> clone() const override
	{
		return std::make_unique<MODSpawner>(*this);
	}
};

} } // namespace libcube::pikmin1
