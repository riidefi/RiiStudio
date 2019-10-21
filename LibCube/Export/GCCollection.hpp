#pragma once

#include <LibRiiEditor/pluginapi/Interface.hpp>
#include <LibCube/GX/Material.hpp>
#include <optional>

namespace libcube {

struct GCCollection : public pl::AbstractInterface
{
	GCCollection() : pl::AbstractInterface(pl::InterfaceID::LibCube_GCCollection)
	{}
	~GCCollection() override = default;

	virtual u32 getNumMaterials() const = 0;

	struct IMaterialDelegate
	{
		virtual ~IMaterialDelegate() = default;

		enum class PropSupport
		{
			Unsupported,
			Read,
			ReadWrite
		};

		struct SupportRegistration
		{
			// FIXME: bitfield
			PropSupport cullMode;
			PropSupport zCompLoc;
			PropSupport zComp;
			PropSupport genInfo;
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

		// Properties must be allocated in the delegate instance.
		virtual gx::CullMode getCullMode() const = 0;
		virtual void setCullMode(gx::CullMode value) = 0;

		struct GenInfoCounts { u8 colorChan, texGen, tevStage, indStage; };
		virtual GenInfoCounts getGenInfo() const = 0;
		virtual void setGenInfo(const GenInfoCounts& value) = 0;

		virtual bool getZCompLoc() const = 0;
		virtual void setZCompLoc(bool value) = 0;

		virtual SupportRegistration getRegistration() const = 0;

		virtual const char* getNameCStr() { return "Unsupported"; }
		virtual void* getRaw() = 0; // For selection
	};

	// Non-owning reference
	virtual IMaterialDelegate& getMaterialDelegate(u32 idx) = 0;
	// For material delegates, if size of materials is altered
	virtual void update() = 0;
};

} // namespace libcube
