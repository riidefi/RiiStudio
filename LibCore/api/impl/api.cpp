
#ifdef _WIN32

#include <Windows.h>
#include <LibCore/api/Node.hpp>

using riimain_fn_t = void (CALLBACK*) (px::PackageInstaller*);

bool installModuleNative(const std::string& path, px::PackageInstaller* pInstaller)
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

#endif



#include "api/Node.hpp"
#include "common.h"
#include <vector>
#include <memory>

px::PackageInstaller* px::PackageInstaller::spInstance;

static bool ends_with(const std::string& value, const std::string& ending)
{
	return ending.size() <= value.size() && std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

struct CorePackageInstaller : px::PackageInstaller
{
    void registerFactory(std::unique_ptr<px::IFactory> factory) override
    {
       mFactories.push_back(std::move(factory));
    }
    void registerMirror(const px::MirrorEntry& entry) override
    {
		px::ReflectionMesh::getInstance()->getDataMesh().enqueueHierarchy(entry);
    }
    void installModule(const std::string& path) override
    {
		if (ends_with(path, ".dll"))
			installModuleNative(path, this);
    }

    void registerSerializer(std::unique_ptr<px::IBinarySerializer> ser) override
    {
        mSerializers.push_back(std::move(ser));
    }

	void registerObject(const px::RichName& details) override
	{
		px::ReflectionMesh::getInstance()->getDataMesh().declare(std::string(details.namespacedId), details);
	}

	px::Dynamic spawnState(const std::string& type)
	{
		for (const auto& it : mFactories)
		{
			if (it->id == type)
				return std::move(it->spawn());
		}

		assert(!"Failed to spawn state..");
		throw "Cannot spawn";
		return px::Dynamic(std::move(std::make_unique<px::IDestructable>()), 0, "");
	}

	std::vector<std::unique_ptr<px::IFactory>> mFactories;
    std::vector<std::unique_ptr<px::IBinarySerializer>> mSerializers;
};

px::ReflectionMesh* px::ReflectionMesh::spInstance;
CorePackageInstaller* spCorePackageInstaller;
px::Dynamic SpawnState(const std::string& type)
{
	return std::move(spCorePackageInstaller->spawnState(type));
}
bool IsConstructable(const std::string& type)
{
	const auto& factories = spCorePackageInstaller->mFactories;
	return std::find_if(factories.begin(), factories.end(), [&](const auto& it) {
		return it->id == type;
	}) != factories.end();
}
px::RichName GetRich(const std::string& type)
{
	const auto* got = px::ReflectionMesh::getInstance()->getDataMesh().get(type);
	assert(got);
	return got->mName;
}
std::pair<std::string, std::unique_ptr<px::IBinarySerializer>> SpawnImporter(const std::string& fileName, oishii::BinaryReader& reader)
{
	std::string match = "";
	std::unique_ptr<px::IBinarySerializer> out = nullptr;

	for (const auto& plugin : spCorePackageInstaller->mSerializers)
	{
		if (plugin->canRead())
		{
			oishii::JumpOut reader_guard(reader, reader.tell());

			match = plugin->matchFile(fileName, reader);

			if (!match.empty())
			{
				out = plugin->clone();
				break;
			}
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
std::unique_ptr<px::IBinarySerializer> SpawnExporter(const std::string& type)
{
	for (const auto& plugin : spCorePackageInstaller->mSerializers)
	{
		if (plugin->matchForWrite(type) && plugin->canWrite())
		{
			return plugin->clone();
		}
	}

	return nullptr;
}
	
struct DataMesh : public px::DataMesh
{
    px::InternalClassMirror* get(const std::string& id) override
    {
		const auto found = mClasses.find(id);
        if (found == mClasses.end())
            return nullptr;
        return &found->second;
    }
    void declare(const std::string& id, px::RichName name) override
    {
        mClasses[id].mName = name;
    }
    void enqueueHierarchy(px::MirrorEntry entry) override
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
    std::map<std::string, px::InternalClassMirror> mClasses;
    std::queue<px::MirrorEntry> mToInsert;
};


struct ReflectionMesh : public px::ReflectionMesh
{
    using px::ReflectionMesh::ReflectionMesh;

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

void InitAPI()
{
	spCorePackageInstaller = new CorePackageInstaller();
    px::PackageInstaller::spInstance = spCorePackageInstaller;
    px::ReflectionMesh::setInstance(new ReflectionMesh(std::make_unique<DataMesh>()));
}
void DeinitAPI()
{
	spCorePackageInstaller = nullptr;
    delete px::PackageInstaller::spInstance;
    delete px::ReflectionMesh::getInstance();
}


