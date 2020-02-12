#pragma once

#include <LibCore/api/Node.hpp>
#include <ThirdParty/glm/vec2.hpp>
#include <ThirdParty/glm/vec3.hpp>
#include <ThirdParty/glm/mat4x4.hpp>

#include <array>
#include <LibCore/common.h>

#include <ThirdParty/fa5/IconsFontAwesome5.h>

// TODO: Interdependency
#include <RiiStudio/editor/renderer/UBOBuilder.hpp>

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
		assert(static_cast<u64>(f) < static_cast<u64>(Feature::Max));

		if (static_cast<u64>(f) >= static_cast<u64>(Feature::Max))
			return Coverage::Unsupported;

		return registration[static_cast<u64>(f)];
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
		assert(static_cast<u64>(f) < static_cast<u64>(Feature::Max));

		if (static_cast<u64>(f) < static_cast<u64>(Feature::Max))
			registration[static_cast<u64>(f)] = s;
	}

	Coverage& operator[](Feature f) noexcept
	{
		assert(static_cast<u64>(f) < static_cast<u64>(Feature::Max));

		return registration[static_cast<u64>(f)];
	}
	Coverage operator[](Feature f) const noexcept
	{
		assert(static_cast<u64>(f) < static_cast<u64>(Feature::Max));

		return registration[static_cast<u64>(f)];
	}
private:
	std::array<Coverage, static_cast<u64>(Feature::Max)> registration;
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


struct Bone : public px::IDestructable
{
	PX_TYPE_INFO_EX("3D Bone", "3d_bone", "3D::Bone", ICON_FA_BONE, ICON_FA_BONE);
	virtual s64 getId() { return -1; }
	virtual void copy(lib3d::Bone& to) {}

	virtual Coverage supportsBoneFeature(BoneFeatures f) { return Coverage::Unsupported; }

	virtual SRT3 getSRT() const = 0;
	virtual void setSRT(const SRT3& srt) = 0;

	virtual s64 getParent() const = 0;
	virtual void setParent(s64 id) = 0;
	virtual u64 getNumChildren() const = 0;
	virtual s64 getChild(u64 idx) const = 0;
	virtual s64 addChild(s64 child) = 0;
	virtual s64 setChild(u64 idx, s64 id) = 0;
	inline s64 removeChild(u64 idx)
	{
		return setChild(idx, -1);
	}

	virtual AABB getAABB() const = 0;
	virtual void setAABB(const AABB& v) = 0;

	virtual float getBoundingRadius() const = 0;
	virtual void setBoundingRadius(float v) = 0;

	virtual bool getSSC() const = 0;
	virtual void setSSC(bool b) = 0;

	struct Display { u32 matId = 0; u32 polyId = 0; u8 prio = 0; };
	virtual u64 getNumDisplays() const = 0;
	virtual Display getDisplay(u64 idx) const = 0;
};
struct Material : public px::IDestructable
{
	PX_TYPE_INFO("3D Material", "3d_material", "3D::Material");


	virtual std::string getName() const { return "Untitled Material"; }
	virtual s64 getId() const { return -1; }

	virtual bool isXluPass() const { return false; }

	virtual std::pair<std::string, std::string> generateShaders() const = 0;
	// TODO: Interdependency
	virtual void generateUniforms(DelegatedUBOBuilder& builder, const glm::mat4& M, const glm::mat4& V, const glm::mat4& P) const = 0;
};

struct Texture : public px::IDestructable
{
	PX_TYPE_INFO("Texture", "tex", "3D::Texture");

	virtual std::string getName() const { return "Untitled Texture"; }
	virtual s64 getId() const { return -1; }

	virtual u32 getDecodedSize(bool mip) const
	{
		return getWidth() * getHeight() * 4;
	}
	virtual u32 getEncodedSize(bool mip) const = 0;
	virtual void decode(std::vector<u8>& out, bool mip) const = 0;

	
	virtual u16 getWidth() const = 0;
	virtual void setWidth(u16 width) = 0;
	virtual u16 getHeight() const = 0;
	virtual void setHeight(u16 height) = 0;
};

struct Polygon : public px::IDestructable
{
	PX_TYPE_INFO("Polygon", "poly", "3D::Polygon");
	virtual ~Polygon() = default;
	
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

	struct SimpleVertex
	{
		// Not all are real. Use hasAttrib()
		u8 evpIdx; // If read from a GC model, not local to mprim
		glm::vec3 position;
		glm::vec3 normal;
		std::array<glm::vec3, 2> colors;
		std::array<glm::vec2, 8> uvs;
	};

	// For now... (slow api)
	virtual void propogate(VBOBuilder& out) const = 0;


	//	virtual SimpleVertex getPrimitiveVertex(u64 prim_idx, u64 vtx_idx) = 0;
	//	virtual void setPrimitiveVertex(u64 prim_idx, u64 vtx_idx, const SimpleVertex& vtx) = 0;

	// Call after any change
	virtual void update() {}
};

} // namespace lib3d
