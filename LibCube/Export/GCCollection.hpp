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

		// Properties must be allocated in the delegate instance.
		virtual Property<gx::CullMode>* getCullMode() = 0;
		// For now, unable to edit inactive elements
		struct GenInfoCounts { u8 colorChan, texGen, tevStage, indStage; };
		virtual Property<GenInfoCounts>* getGenInfo() = 0;

		virtual Property<bool>* getZCompLoc() = 0;

		virtual SupportRegistration getRegistration() const = 0;
	};

	virtual std::unique_ptr<IMaterialDelegate> getMaterialDelegate(u32 idx) = 0;
};

} // namespace libcube
