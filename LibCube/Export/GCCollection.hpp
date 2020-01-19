#pragma once

#include <LibRiiEditor/pluginapi/Interface.hpp>
#include <LibCube/GX/Material.hpp>
#include <optional>
#include <ThirdParty/glm/vec3.hpp>
#include <LibCube/Common/BoundBox.hpp>
#include <Lib3D/interface/i3dmodel.hpp>
#include <map>
namespace libcube {


template<typename Feature>
struct TPropertySupport
{
	enum class Coverage
	{
		Unsupported,
		Read,
		ReadWrite
	};


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

/*
template<typename T>
struct Property
{
	virtual ~Property() = default;
	//	bool writeable = true;

	virtual T get() = 0;
	virtual void set(T) = 0;
};

template<typename T>
struct RefProperty : public Property<T>
{
	RefProperty(T& r) : mRef(r) {}
	~RefProperty() override = default;

	T get() override { return mRef; }
	void set(T v) override { mRef = v; };

	T& mRef;
};
*/
struct IndexedVertex
{
	inline const u16& operator[] (gx::VertexAttribute attr) const
	{
		assert((u32)attr < (u32)gx::VertexAttribute::Max);
		return indices[(u32)attr];
	}
	inline u16& operator[] (gx::VertexAttribute attr)
	{
		assert((u32)attr < (u32)gx::VertexAttribute::Max);
		return indices[(u32)attr];
	}

private:
	std::array<u16, (u32)gx::VertexAttribute::Max> indices;
};
struct IndexedPrimitive
{
	gx::PrimitiveType mType;
	std::vector<IndexedVertex> mVertices;

	IndexedPrimitive() = default;
	IndexedPrimitive(gx::PrimitiveType type, u32 size)
		: mType(type), mVertices(size)
	{
	}
};
struct VertexDescriptor
{
	std::map<gx::VertexAttribute, gx::VertexAttributeType> mAttributes;
	u32 mBitfield; // values of VertexDescriptor

	u32 getVcdSize() const
	{
		u32 vcd_size = 0;
		for (int i = 0; i < (int)gx::VertexAttribute::Max; ++i)
			if (mBitfield & (1 << 1))
				++vcd_size;
		return vcd_size;
	}
	void calcVertexDescriptorFromAttributeList()
	{
		mBitfield = 0;
		for (gx::VertexAttribute i = gx::VertexAttribute::PositionNormalMatrixIndex;
			static_cast<u32>(i) < static_cast<u32>(gx::VertexAttribute::Max);
			i = static_cast<gx::VertexAttribute>(static_cast<u32>(i) + 1))
		{
			auto found = mAttributes.find(i);
			if (found != mAttributes.end() && found->second != gx::VertexAttributeType::None)
				mBitfield |= (1 << static_cast<u32>(i));
		}
	}
	bool operator[] (gx::VertexAttribute attr) const
	{
		return mBitfield & (1 << static_cast<u32>(attr));
	}
};
inline gx::VertexAttribute operator+(gx::VertexAttribute attr, u32 i)
{
	u32 out = static_cast<u32>(attr) + i;
	assert(out < static_cast<u32>(gx::VertexAttribute::Max));
	return static_cast<gx::VertexAttribute>(out);
}
struct IndexedPolygon : public lib3d::Polygon
{
	// In wii/gc, absolute indices across mprims
	u32 getNumPrimitives() const override
	{
		u32 total = 0;

		for (u32 i = 0; i < getNumMatrixPrimitives(); ++i)
			total += getMatrixPrimitiveNumIndexedPrimitive(i);

		return total;
	}
	// Triangles
	// We add this to the last mprim. May need to be split up later.
	int addPrimitive() override
	{
		if (getNumMatrixPrimitives() == 0)
			addMatrixPrimitive();

		const int idx = getNumMatrixPrimitives();
		assert(idx > 0);
		if (idx <= 0)
			return -1;

		//	const int iprim_idx = addMatrixPrimitiveIndexedPrimitive();
		//	assert(iprim_idx >= 0);
		//	if (iprim_idx < 0)
		//		return;
		//	
		//	auto& iprim = getMatrixPrimitiveIndexedPrimitive(idx, iprim_idx);
		//	
		//	// TODO
		return -1;
	}
	bool hasAttrib(SimpleAttrib attrib) const override
	{
		switch (attrib)
		{
		case SimpleAttrib::EnvelopeIndex:
			return getVcd()[gx::VertexAttribute::PositionNormalMatrixIndex];
		case SimpleAttrib::Position:
			return getVcd()[gx::VertexAttribute::Position];
		case SimpleAttrib::Normal:
			return getVcd()[gx::VertexAttribute::Position];
		case SimpleAttrib::Color0:
		case SimpleAttrib::Color1:
			return getVcd()[gx::VertexAttribute::Color0 + ((u32)attrib - (u32)SimpleAttrib::Color0)];
		case SimpleAttrib::TexCoord0:
		case SimpleAttrib::TexCoord1:
		case SimpleAttrib::TexCoord2:
		case SimpleAttrib::TexCoord3:
		case SimpleAttrib::TexCoord4:
		case SimpleAttrib::TexCoord5:
		case SimpleAttrib::TexCoord6:
		case SimpleAttrib::TexCoord7:
			return getVcd()[gx::VertexAttribute::TexCoord0 + ((u32)attrib - (u32)SimpleAttrib::TexCoord0)];
		default:
			return false;
		}
	}
	void setAttrib(SimpleAttrib attrib, bool v) override
	{
		// We usually recompute the index (no direct support*)
		switch (attrib)
		{
		case SimpleAttrib::EnvelopeIndex:
			getVcd().mAttributes[gx::VertexAttribute::PositionNormalMatrixIndex] = gx::VertexAttributeType::Direct;
			break;
		case SimpleAttrib::Position:
			getVcd().mAttributes[gx::VertexAttribute::Position] = gx::VertexAttributeType::Short;
			break;
		case SimpleAttrib::Normal:
			getVcd().mAttributes[gx::VertexAttribute::Position] = gx::VertexAttributeType::Short;
			break;
		case SimpleAttrib::Color0:
		case SimpleAttrib::Color1:
			getVcd().mAttributes[gx::VertexAttribute::Color0 + ((u32)attrib - (u32)SimpleAttrib::Color0)] = gx::VertexAttributeType::Short;
			break;
		case SimpleAttrib::TexCoord0:
		case SimpleAttrib::TexCoord1:
		case SimpleAttrib::TexCoord2:
		case SimpleAttrib::TexCoord3:
		case SimpleAttrib::TexCoord4:
		case SimpleAttrib::TexCoord5:
		case SimpleAttrib::TexCoord6:
		case SimpleAttrib::TexCoord7:
			getVcd().mAttributes[gx::VertexAttribute::TexCoord0 + ((u32)attrib - (u32)SimpleAttrib::TexCoord0)] = gx::VertexAttributeType::Short;
			break;
		default:
			assert(!"Invalid simple vertex attrib");
			break;
		}
	}
	IndexedPrimitive& getIndexedPrimitiveFromSuperIndex(u32 idx)
	{
		return *(IndexedPrimitive*)nullptr;
	}
	const IndexedPrimitive& getIndexedPrimitiveFromSuperIndex(u32 idx) const
	{
		return *(IndexedPrimitive*)nullptr;
	}
	u32 getPrimitiveVertexCount(u32 index) const override
	{
		return getIndexedPrimitiveFromSuperIndex(index).mVertices.size();
	}
	void resizePrimitiveVertexArray(u32 index, u32 size) override
	{
		getIndexedPrimitiveFromSuperIndex(index).mVertices.resize(size);
	}
	SimpleVertex getPrimitiveVertex(u32 prim_idx, u32 vtx_idx) override
	{
		const auto& iprim = getIndexedPrimitiveFromSuperIndex(prim_idx);
		assert(vtx_idx < iprim.mVertices.size());
		const auto& vtx = iprim.mVertices[vtx_idx];

		//return {
		//	vtx[gx::VertexAttribute::PositionNormalMatrixIndex],
		//	vtx[gx::VertexAttribute::Position]
		//};
		return {};
	}
	// We add on to the attached buffer
	void setPrimitiveVertex(u32 prim_idx, u32 vtx_idx, const SimpleVertex& vtx) override
	{

	}
	void update() override
	{
		// Split up added primitives if necessary
	}


	virtual u32 getNumMatrixPrimitives() const = 0;
	virtual s32 addMatrixPrimitive() = 0;
	virtual s16 getMatrixPrimitiveCurrentMatrix(u32 idx) const = 0;
	virtual void setMatrixPrimitiveCurrentMatrix(u32 idx, s16 mtx) = 0;
	// Matrix list access
	virtual u32 getMatrixPrimitiveNumIndexedPrimitive(u32 idx) const = 0;
	virtual const IndexedPrimitive& getMatrixPrimitiveIndexedPrimitive(u32 idx, u32 prim_idx) const = 0;
	virtual IndexedPrimitive& getMatrixPrimitiveIndexedPrimitive(u32 idx, u32 prim_idx) = 0;

	virtual VertexDescriptor& getVcd() = 0;
	virtual const VertexDescriptor& getVcd() const = 0;

};
struct GCCollection : public lib3d::I3DModel
{
	GCCollection()
	{
		mGcXfs.mStack.push_back(std::make_unique<pl::TransformStack::XForm>(
			pl::RichName{"This is a test", "test", "test"}
		));
	}
	virtual ~GCCollection() = default;

	pl::TransformStack mGcXfs;

	struct IMaterialDelegate : public lib3d::Material
	{
		virtual ~IMaterialDelegate() = default;


		enum class Feature
		{
			CullMode,
			ZCompareLoc,
			ZCompare,
			GenInfo,

			MatAmbColor,

			Max
		};
		// Compat
		struct PropertySupport : public TPropertySupport<Feature>
		{
			using Feature = Feature;
			static constexpr std::array<const char*, (u32)Feature::Max> featureStrings = {
				"Culling Mode",
				"Early Z Comparison",
				"Z Comparison",
				"GenInfo",
				"Material/Ambient Colors"
			};
		};
		
		PropertySupport support;

		// Properties must be allocated in the delegate instance.
		virtual gx::CullMode getCullMode() const = 0;
		virtual void setCullMode(gx::CullMode value) = 0;

		struct GenInfoCounts { u8 colorChan, texGen, tevStage, indStage; };
		virtual GenInfoCounts getGenInfo() const = 0;
		virtual void setGenInfo(const GenInfoCounts& value) = 0;

		virtual bool getZCompLoc() const = 0;
		virtual void setZCompLoc(bool value) = 0;
		// Always assumed out of 2
		virtual gx::Color getMatColor(u32 idx) const = 0;
		virtual gx::Color getAmbColor(u32 idx) const = 0;
		virtual void setMatColor(u32 idx, gx::Color v) = 0;
		virtual void setAmbColor(u32 idx, gx::Color v) = 0;

		virtual std::string getName() const = 0;
		virtual void* getRaw() = 0; // For selection
	};
	struct IBoneDelegate : public lib3d::Bone
	{
		enum class Billboard
		{
			None,
			/* G3D Standard */
			Z_Parallel,
			Z_Face, // persp
			Z_ParallelRotate,
			Z_FaceRotate,
			/* G3D Y*/
			Y_Parallel,
			Y_Face, // persp

			// TODO -- merge
			J3D_XY,
			J3D_Y
		};
		virtual Billboard getBillboard() const = 0;
		virtual void setBillboard(Billboard b) = 0;
		// For extendeds
		virtual int getBillboardAncestor() { return -1; }
		virtual void setBillboardAncestor(int ancestor_id) {}

		void copy(lib3d::Bone& to) override
		{
			lib3d::Bone::copy(to);
			IBoneDelegate* bone = reinterpret_cast<IBoneDelegate*>(&to);
			if (bone)
			{
				if (bone->supportsBoneFeature(lib3d::BoneFeatures::StandardBillboards) == lib3d::Coverage::ReadWrite)
					bone->setBillboard(getBillboard());
				if (bone->supportsBoneFeature(lib3d::BoneFeatures::ExtendedBillboards) == lib3d::Coverage::ReadWrite)
					bone->setBillboardAncestor(getBillboardAncestor());
			}
		}
		inline bool canWrite(lib3d::BoneFeatures f)
		{
			return supportsBoneFeature(f) == lib3d::Coverage::ReadWrite;
		}
	};
	virtual IBoneDelegate& getBone(u32 idx) override = 0;
	virtual IMaterialDelegate& getMaterial(u32 idx) override = 0;
};



} // namespace libcube
