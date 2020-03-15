#ifdef _WIN32
#include <string>
#include <Windows.h>
#endif


#include "common.h"
#include <vector>
#include <memory>
#include <core/kpi/Node.hpp>
#include <oishii/oishii/reader/binary_reader.hxx>


#ifdef _WIN32

using riimain_fn_t = void (CALLBACK*) (kpi::ApplicationPlugins*);

bool installModuleNative(const std::string& path, kpi::ApplicationPlugins* pInstaller)
{
	auto hnd = LoadLibraryA(path.c_str());

	if (!hnd)
		return false;

	auto fn = reinterpret_cast<riimain_fn_t>(GetProcAddress(hnd, "__riimain"));
	if (!fn)
		return false;

	fn(pInstaller);
	return true;
}
#else
#warning "No module installation support.."
template<typename... args>
bool installModuleNative(args...)
{
	return false;
}
#endif




kpi::ApplicationPlugins* kpi::ApplicationPlugins::spInstance;

static bool ends_with(const std::string& value, const std::string& ending)
{
	return ending.size() <= value.size() && std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

struct CorePackageInstaller : kpi::ApplicationPlugins
{
    void registerMirror(const kpi::MirrorEntry& entry) override
    {
		kpi::ReflectionMesh::getInstance()->getDataMesh().enqueueHierarchy(entry);
    }
    void installModule(const std::string& path) override
    {
		if (ends_with(path, ".dll"))
			installModuleNative(path, this);
    }


	std::unique_ptr<kpi::IDocumentNode> spawnState(const std::string& type) const
	{
		for (const auto& it : mFactories)
		{
			if (it.first == type)
				return it.second->spawn();
		}
	
		assert(!"Failed to spawn state..");
		throw "Cannot spawn";
		return nullptr;
	}
	std::unique_ptr<kpi::IDocumentNode> constructObject(const std::string& type, kpi::IDocumentNode* parent = nullptr) const override
	{
		auto spawned = spawnState(type);
		spawned->parent = parent;
		return spawned;
	}
};

kpi::ReflectionMesh* kpi::ReflectionMesh::spInstance;
CorePackageInstaller* spCorePackageInstaller;
std::unique_ptr<kpi::IDocumentNode> SpawnState(const std::string& type)
{
	return spCorePackageInstaller->spawnState(type);
}
bool IsConstructible(const std::string& type)
{
	const auto& factories = spCorePackageInstaller->mFactories;
	return std::find_if(factories.begin(), factories.end(), [&](const auto& it) {
		return it.second->getId() == type;
	}) != factories.end();
}
kpi::RichName GetRich(const std::string& type)
{
	const auto* got = kpi::ReflectionMesh::getInstance()->getDataMesh().get(type);
	assert(got);
	return got->mName;
}

std::pair<std::string, std::unique_ptr<kpi::IBinaryDeserializer>> SpawnImporter(const std::string& fileName, oishii::BinaryReader& reader)
{
	std::string match = "";
	std::unique_ptr<kpi::IBinaryDeserializer> out = nullptr;

	assert(spCorePackageInstaller);
	for (const auto& plugin : spCorePackageInstaller->mReaders)
	{
		oishii::JumpOut reader_guard(reader, reader.tell());
		match = plugin->canRead_(fileName, reader);
		if (!match.empty())
		{
			out = plugin->clone();
			break;
		}
	}

	if (match.empty())
	{
		DebugReport("No matches.\n");
		return {};
	}
	else
	{
		DebugReport("Success spawning importer\n");
		return {
			match,
			std::move(out)
		};
	}
}
#if 0
std::unique_ptr<kpi::IBinarySerializer> SpawnExporter(const std::string& type)
{
	for (const auto& plugin : spCorePackageInstaller->mWriters)
	{
		if (plugin->canWrite_(type))
		{
			return plugin->clone();
		}
	}

	return nullptr;
}
#endif
struct DataMesh : public kpi::DataMesh
{
    kpi::InternalClassMirror* get(const std::string& id) override
    {
		const auto found = mClasses.find(id);
        if (found == mClasses.end())
            return nullptr;
        return &found->second;
    }
    void declare(const std::string& id, kpi::RichName name) override
    {
        mClasses[id].mName = name;
    }
    void enqueueHierarchy(kpi::MirrorEntry entry) override
    {
		assert(entry.derived != entry.base);
        mToInsert.push(entry);
    }
    void compute() override
    {
        while (!mToInsert.empty())
        {
            const auto& cmd = mToInsert.front();

            // Always ensure the data mesh is in sync with the rest
            if (mClasses.find(std::string(cmd.base)) == mClasses.end())
            {
                printf("Warning: Type %s references undefined parent %s.\n", cmd.derived.data(), cmd.base.data());
            }
 //           else
            {
                mClasses[std::string(cmd.base)].derived = cmd.base;
                mClasses[std::string(cmd.base)].mChildren.push_back(std::string(cmd.derived));
            }
            // TODO
            mClasses[std::string(cmd.derived)].derived = cmd.derived;

			assert(cmd.derived != cmd.base);

            mClasses[std::string(cmd.derived)].mParents.push_back({ std::string(cmd.base), cmd.translation });
            mToInsert.pop();
        }
    }
private:
    // File states and interfaces -- hierarchy.
    std::map<std::string, kpi::InternalClassMirror> mClasses;
    std::queue<kpi::MirrorEntry> mToInsert;
};


struct ReflectionMesh : public kpi::ReflectionMesh
{
    using kpi::ReflectionMesh::ReflectionMesh;

    ReflectionInfoHandle lookupInfo(std::string info) override
	{
		return ReflectionInfoHandle(&getDataMesh(), info);
	}
	void findParentOfType(std::vector<void*>& out,
        void* in, const std::string& info, const std::string& key) override
	{
		auto hnd = ReflectionInfoHandle(&getDataMesh(), info);

		for (int i = 0; i < hnd.getNumParents(); ++i)
		{
			auto parent = hnd.getParent(i);
			assert(hnd.getName() != parent.getName());
			if (parent.getName() == key)
			{
				char* new_ = reinterpret_cast<char*>(in) + hnd.getTranslationForParent(i);
				if (std::find(out.begin(), out.end(), (void*)new_) == out.end())
					out.push_back(new_);
			}
			else
				findParentOfType(out, reinterpret_cast<char*>(in) + hnd.getTranslationForParent(i), parent.getName(), key);
		}
	}
};

#include <core/3d/i3dmodel.hpp>

void InitAPI()
{
	spCorePackageInstaller = new CorePackageInstaller();
    kpi::ApplicationPlugins::spInstance = spCorePackageInstaller;
    kpi::ReflectionMesh::setInstance(new ReflectionMesh(std::make_unique<DataMesh>()));

	kpi::ApplicationPlugins& installer = *kpi::ApplicationPlugins::spInstance;

	// Scene registers
}
void DeinitAPI()
{
	spCorePackageInstaller = nullptr;
    delete kpi::ApplicationPlugins::spInstance;
    delete kpi::ReflectionMesh::getInstance();
}
