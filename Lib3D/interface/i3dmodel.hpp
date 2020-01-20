#pragma once

#include <LibRiiEditor/pluginapi/Plugin.hpp>
#include <ThirdParty/glm/vec2.hpp>
#include <ThirdParty/glm/vec3.hpp>

namespace lib3d {
enum class Coverage
{
	Unsupported,
	Read,
	ReadWrite
};

template<typename Feature>
struct TPropertySupport
{
	


	Coverage supports(Feature f) const noexcept
	{
		assert(static_cast<u32>(f) < static_cast<u32>(Feature::Max));

		if (static_cast<u32>(f) >= static_cast<u32>(Feature::Max))
			return Coverage::Unsupported;

		return registration[static_cast<u32>(f)];
	}
	bool canRead(Feature f) const noexcept
	{
		return supports(f) == Coverage::Read || supports(f) == Coverage::ReadWrite;
	}
	bool canWrite(Feature f) const noexcept
	{
		return supports(f) == Coverage::ReadWrite;
	}
	void setSupport(Feature f, Coverage s)
	{
		assert(static_cast<u32>(f) < static_cast<u32>(Feature::Max));

		if (static_cast<u32>(f) < static_cast<u32>(Feature::Max))
			registration[static_cast<u32>(f)] = s;
	}

	Coverage& operator[](Feature f) noexcept
	{
		assert(static_cast<u32>(f) < static_cast<u32>(Feature::Max));

		return registration[static_cast<u32>(f)];
	}
	Coverage operator[](Feature f) const noexcept
	{
		assert(static_cast<u32>(f) < static_cast<u32>(Feature::Max));

		return registration[static_cast<u32>(f)];
	}
private:
	std::array<Coverage, static_cast<u32>(Feature::Max)> registration;
};

enum class I3DFeature
{
	Bone,
	Max
};
enum class BoneFeatures
{
	// -- Standard features: XForm, Hierarchy. Here for read-only access
	SRT,
	Hierarchy,
	// -- Optional features
	StandardBillboards, // J3D
	ExtendedBillboards, // G3D
	AABB,
	BoundingSphere,
	SegmentScaleCompensation, // Maya
	// Not exposed currently:
	//	ModelMatrix,
	//	InvModelMatrix
	Max
};
struct AABB
{
	glm::vec3 min, max;
};
struct SRT3
{
	glm::vec3 scale;
	glm::vec3 rotation;
	glm::vec3 translation;

	bool operator==(const SRT3& rhs) const
	{
		return scale == rhs.scale && rotation == rhs.rotation && translation == rhs.translation;
	}
	bool operator!=(const SRT3& rhs) const
	{
		return !operator==(rhs);
	}
};


struct Bone
{
	virtual ~Bone() = default;
	
	virtual std::string getName() { return "Untitled Bone"; }
	virtual int getId() { return -1; }
	virtual void copy(lib3d::Bone& to) {}

	virtual Coverage supportsBoneFeature(BoneFeatures f) { return Coverage::Unsupported; }

	virtual SRT3 getSRT() const = 0;
	virtual void setSRT(const SRT3& srt) = 0;

	virtual int getParent() const = 0;
	virtual void setParent(int id) = 0;
	virtual u32 getNumChildren() const = 0;
	virtual int getChild(u32 idx) const = 0;
	virtual int addChild(int child) = 0;
	virtual int setChild(u32 idx, int id) = 0;
	inline int removeChild(u32 idx)
	{
		return setChild(idx, -1);
	}

	virtual AABB getAABB() const = 0;
	virtual void setAABB(const AABB& v) = 0;

	virtual float getBoundingRadius() const = 0;
	virtual void setBoundingRadius(float v) = 0;

	virtual bool getSSC() const = 0;
	virtual void setSSC(bool b) = 0;


};
struct Material
{
	virtual ~Material() = default;

	virtual std::string getName() const { return "Untitled Material"; }
	virtual int getId() const { return -1; }
};

struct Polygon
{
	virtual ~Polygon() = default;

	// Bounding volumes...

	// In wii/gc, absolute indices across mprims
	virtual u32 getNumPrimitives() const = 0;
	// Assume triangles
	virtual int addPrimitive() = 0;

	enum class SimpleAttrib
	{
		EnvelopeIndex, // u8
		Position, // vec3
		Normal, // vec3
		Color0, // rgba8
		Color1,
		TexCoord0, // vec2
		TexCoord1,
		TexCoord2,
		TexCoord3,
		TexCoord4,
		TexCoord5,
		TexCoord6,
		TexCoord7,
		Max
	};
	virtual bool hasAttrib(SimpleAttrib attrib) const = 0;
	virtual void setAttrib(SimpleAttrib attrib, bool v) = 0;

	virtual u32 getPrimitiveVertexCount(u32 index) const = 0;
	virtual void resizePrimitiveVertexArray(u32 index, u32 size) = 0;

	struct SimpleVertex
	{
		// Not all are real. Use hasAttrib()
		u8 evpIdx; // If read from a GC model, not local to mprim
		glm::vec3 position;
		glm::vec3 normal;
		std::array<u32, 2> colors;
		std::array<glm::vec2, 8> uvs;
	};

	virtual SimpleVertex getPrimitiveVertex(u32 prim_idx, u32 vtx_idx) = 0;
	virtual void setPrimitiveVertex(u32 prim_idx, u32 vtx_idx, const SimpleVertex& vtx) = 0;

	// Call after any change
	virtual void update() {}
};

// For now, model + texture
struct I3DModel
{
	virtual ~I3DModel() = default;

	virtual u32 getNumBones() const = 0;
	virtual Bone& getBone(u32 idx) = 0;
	virtual int addBone() = 0;

	virtual u32 getNumMaterials() const = 0;
	virtual Material& getMaterial(u32 idx) = 0;
	virtual int addMaterial() = 0;
};

} // namespace lib3d
