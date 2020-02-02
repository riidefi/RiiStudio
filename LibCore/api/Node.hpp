#pragma once

#include "RichName.hpp"
#include "Reflection.hpp"

#include <memory>
#include <string_view>

#include <oishii/reader/binary_reader.hxx>
#include <oishii/v2/writer/binary_writer.hxx>

namespace px {

/*
Before we had filestates, filestatespawnres, importers, exporters and interfaces

Now:
    * File state is composed of folders of nodes--allows for nice interop between languages.
    * Node -> IBinarySerializable (Importer/Exporter)
    * All nodes are reflected like old interfaces.
    * Old spawners replaced by generic factories.
*/

#define PX_TYPE_INFO_EX(gui, cli, internal, icon_pl, icon_sg) \
    constexpr static px::RichName TypeInfo = { gui, internal, cli, {icon_pl, icon_sg } }
#define PX_TYPE_INFO(gui, cli, internal) \
    constexpr static px::RichName TypeInfo = { gui, internal, cli }

// More like IObject
struct IDestructable
{
    virtual ~IDestructable() = default;

	virtual std::string getName() const { return "Untitled"; }

	PX_TYPE_INFO_EX("Object", "obj", "IDestructable", "(?)", "(?)");
};

//! Our solution to cross-language interop.
//!
struct Dynamic
{
    std::unique_ptr<IDestructable> mOwner;
    void* mBase;
    std::string mType;

	Dynamic(std::unique_ptr<IDestructable> owner, void* base, const std::string& type)
		: mOwner(std::move(owner)), mBase(base), mType(type)
	{}
};

template<typename T, typename... Args>
Dynamic make_dynamic(Args... args)
{
    auto constructed = std::make_unique<T>(args...);
    void* base = constructed.get();
    return std::move(Dynamic(std::move(constructed), base, std::string(T::TypeInfo.namespacedId)));
}


struct IBinarySerializer : public IDestructable
{
    PX_TYPE_INFO("Serializable Binary", "binary", "IBinarySerializer");

	virtual std::unique_ptr<IBinarySerializer> clone() = 0;

	virtual bool canRead() { return false; }
	virtual bool canWrite() { return false; }

	virtual void read(Dynamic& state, oishii::BinaryReader& reader) {}
	virtual void write(Dynamic& state, oishii::v2::Writer& writer) {}

	virtual std::string matchFile(const std::string& file, oishii::BinaryReader& reader) { return ""; }
};

struct IFactory : public IDestructable
{
    virtual std::unique_ptr<IFactory> clone() = 0;
    virtual Dynamic spawn() = 0;
	std::string_view id;
};

struct PackageInstaller
{
    // DLLs implement their own global instance
    static PackageInstaller* spInstance;

	virtual void registerObject(const RichName& details) = 0;
    virtual void registerFactory(std::unique_ptr<IFactory> factory) = 0;
    virtual void registerSerializer(std::unique_ptr<IBinarySerializer> ser) = 0;
    virtual void registerMirror(const MirrorEntry& entry) = 0;
    virtual void installModule(const std::string& path) = 0;

    template<typename D, typename B>
    void registerParent()
    {
        registerMirror({ D::TypeInfo.namespacedId, B::TypeInfo.namespacedId, computeTranslation<D, B>()});
    }
	template<typename T, typename M>
	void registerMember(int slide)
	{
		registerMirror({ D::TypeInfo.namespacedId, B::TypeInfo.namespacedId, slide });
	}
};

}
