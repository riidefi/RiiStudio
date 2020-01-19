#pragma once

#include <LibRiiEditor/common.hpp>
#include <vector>
#include <string>
#include <ThirdParty/glm/glm.hpp>

#include <LibCube/Common/BoundBox.hpp>
#include <Lib3D/interface/i3dmodel.hpp>
#include <LibCube/Export/GCCollection.hpp>

namespace libcube::jsystem {

template<typename T>
using ID = int;
struct Material;
struct Shape;
struct Joint;
struct JointData
{
	// Four LSBs of flag; left is matrix type
	enum class MatrixType : u16
	{
		Standard = 0,
		Billboard,
		BillboardY
	};

	std::string name;
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
	//glm::mat4x4
	std::array<float, 12>
	inverseBindPoseMtx;
};

struct Joint final : public GCCollection::IBoneDelegate, public JointData
{
	std::string getName() override { return name; }
	int getId() override { return id; }
	void copy(lib3d::Bone& to) override
	{
		GCCollection::IBoneDelegate::copy(to);
		Joint* pJoint = reinterpret_cast<Joint*>(&to);
		if (pJoint)
		{
			name = pJoint->name;
			flag = pJoint->flag;
		}

	}
	lib3d::Coverage supportsBoneFeature(lib3d::BoneFeatures f) override
	{
		switch (f)
		{
		case lib3d::BoneFeatures::SRT:
		case lib3d::BoneFeatures::Hierarchy:
		case lib3d::BoneFeatures::StandardBillboards:
		case lib3d::BoneFeatures::AABB:
		case lib3d::BoneFeatures::BoundingSphere:
		case lib3d::BoneFeatures::SegmentScaleCompensation:
			return lib3d::Coverage::ReadWrite;
		default:
			return lib3d::Coverage::Unsupported;

		}
	}

	lib3d::SRT3 getSRT() const override {
		return { scale, rotate, translate };
	}
	void setSRT(const lib3d::SRT3& srt) override {
		scale = srt.scale;
		rotate = srt.rotation;
		translate = srt.translation;
	}
	int getParent() const override { return parent; }
	void setParent(int id) override { parent = id; }
	u32 getNumChildren() const override { return children.size(); }
	int getChild(u32 idx) const override { return idx < children.size() ? children[idx] : -1; }
	int addChild(int child) override { children.push_back(child); return children.size() - 1; }
	int setChild(u32 idx, int id) override {
		int old = -1;
		if (idx < children.size()) { old = children[idx]; children[idx] = id; }
		return old;
	}
	lib3d::AABB getAABB() const override { return { boundingBox.m_minBounds, boundingBox.m_maxBounds }; }
	void setAABB(const lib3d::AABB& aabb) override { boundingBox = { aabb.min, aabb.max }; }
	float getBoundingRadius() const override { return boundingSphereRadius; }
	void setBoundingRadius(float r) override { boundingSphereRadius = r; }
	bool getSSC() const override { return mayaSSC; }
	void setSSC(bool b) override { mayaSSC = b; }


	Billboard getBillboard() const override
	{
		switch (bbMtxType)
		{
		case MatrixType::Standard:
			return Billboard::None;
		case MatrixType::Billboard:
			return Billboard::J3D_XY;
		case MatrixType::BillboardY:
			return Billboard::J3D_Y;
		default:
			return Billboard::None;
		}
	}
	void setBillboard(Billboard b) override
	{
		switch (b)
		{
		case Billboard::None:
			bbMtxType = MatrixType::Standard;
			break;
		case Billboard::J3D_XY:
			bbMtxType = MatrixType::Billboard;
			break;
		case Billboard::J3D_Y:
			bbMtxType = MatrixType::BillboardY;
			return ;
		default:
			break;
		}
	}

};

}
