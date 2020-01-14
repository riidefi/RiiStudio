#pragma once

#include <LibRiiEditor/pluginapi/Interface.hpp>
#include <LibCube/GX/Material.hpp>
#include <optional>
#include <ThirdParty/glm/vec3.hpp>
#include <LibCube/Common/BoundBox.hpp>

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
struct GCCollection
{
	GCCollection()
	{
		mGcXfs.mStack.push_back(std::make_unique<pl::TransformStack::XForm>(
			pl::RichName{"This is a test", "test", "test"}
		));
	}
	virtual ~GCCollection() = default;

	virtual u32 getNumMaterials() const = 0;
	virtual u32 getNumBones() const = 0;

	pl::TransformStack mGcXfs;

	struct IMaterialDelegate
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

		virtual const char* getNameCStr() { return "Unsupported"; }
		virtual void* getRaw() = 0; // For selection
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
	struct IBoneDelegate : public TPropertySupport<BoneFeatures>
	{
		virtual ~IBoneDelegate() = default;
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

		virtual const char* getNameCStr() { return "-"; }
		// flags not here
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
		virtual SRT3 getSRT() const = 0;
		virtual void setSRT(const SRT3& srt) = 0;

		virtual std::string getParent() const = 0;
		virtual std::vector<std::string> getChildren() const = 0;

		virtual Billboard getBillboard() const = 0;
		virtual void setBillboard(Billboard b) = 0;
		// TODO -- extended requires a reference ancestor bone

		virtual AABB getAABB() const = 0;
		virtual void setAABB(const AABB& v) = 0;

		virtual float getBoundingRadius() const = 0;
		virtual void setBoundingRadius(float v) = 0;

		virtual bool getSSC() const = 0;
		virtual void setSSC(bool b) = 0;
	};
	virtual IBoneDelegate& getBoneDelegate(u32 idx) = 0;
	// Non-owning reference
	virtual IMaterialDelegate& getMaterialDelegate(u32 idx) = 0;
	// For material delegates, if size of materials is altered
	virtual void update() = 0;

	virtual int boneNameToIdx(std::string name) const = 0;
};

} // namespace libcube
