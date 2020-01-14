#pragma once

#include "LibRiiEditor/pluginapi/Plugin.hpp"
#include <map>
#include <mutex>
#include <optional>
#include <vector>
#include <queue>


//! @brief Manages all applet plugins.
//!
class PluginFactory
{
public:
	PluginFactory();
	~PluginFactory() = default;

	//! @brief	Attempt to register a plugin based on its registration details.
	//!
	//! @return If the operation was successful.
	//!
	bool registerPlugin(const pl::Package& package);

	struct SpawnedImporter
	{
		std::string fileStateId;
		std::unique_ptr<pl::Importer> importer;
	};

	std::optional<SpawnedImporter> spawnImporter(const std::string& filename, oishii::BinaryReader& reader);
	// FIXME
	std::unique_ptr<pl::Exporter> spawnExporter(const std::string& id)
	{
		std::lock_guard g(mMutex);

		for (const auto& it : mExporters)
			if (it->match(id))
				return it->spawn();

		return nullptr;
	}
	std::unique_ptr<pl::FileState> spawnFileState(const std::string& fileStateId);

	// Install a DLL
	bool installModule(const std::string& path);

	void computeDataMesh();



private:
	std::mutex mMutex; //!< When performing write operations (registering a plugin)

	//! Other data here references by index -- be careful to maintain that.
	std::vector<std::unique_ptr<pl::FileStateSpawner>> mPlugins;
	std::vector<std::unique_ptr<pl::ImporterSpawner>> mImporters;
	std::vector<std::unique_ptr<pl::ExporterSpawner>> mExporters;

	//
	// PROTOTYPE REFLECTION INTERFACE
	//

public:
	enum class NodeType
	{
		Invalid,
		Importer,
		Exporter,
		FileState,
		Interface
	};
private:
	struct InternalClassMirror
	{
		std::string derived;
		NodeType mType;
		pl::RichName mName;
		u32 mInterfaceFlag;

		struct Entry
		{
			const std::string parent;
			int translation;

			void* cast(void* base) const
			{
				return (char*)base + translation;
			}
		};
		std::vector<Entry> mParents;
		std::vector<std::string> mChildren;
	};

	struct DataMesh
	{
		InternalClassMirror* get(const std::string& id)
		{
			if (mClasses.find(id) == mClasses.end())
				return nullptr;
			return &mClasses[id];
		}
		void declare(const std::string& id, NodeType type, pl::RichName name, u32 flag = 0)
		{
			mClasses[id].mType = type;
			mClasses[id].mName = name;
			mClasses[id].mInterfaceFlag = (type == NodeType::Interface) ? flag : 0;

		}
		void enqueueHierarchy(pl::MirrorEntry entry)
		{
			mToInsert.push(entry);
		}
		void compute();
	private:
		// File states and interfaces -- hierarchy.
		std::map<std::string, InternalClassMirror> mClasses;
		std::queue<pl::MirrorEntry> mToInsert;
	};
	DataMesh mDataMesh;
public:
	class ReflectionInfoHandle
	{
		friend class PluginFactory;
	public:

		bool valid() const {
			return mMirror != nullptr && mMesh != nullptr;
		}
		operator bool() const {
			return valid();
		}

		NodeType getType() const
		{
			return mType;
		}
		std::string getName() const
		{
			if (!valid()) return "";
			return mMirror->derived;
		}
		int getNumParents() const
		{
			if (!valid()) return 0;
			return mMirror->mParents.size();
		}
		ReflectionInfoHandle getParent(int index) const
		{
			if (!valid() || index > mMirror->mParents.size())
				return { NodeType::Invalid };
			return ReflectionInfoHandle(mMesh, mMirror->mParents[index].parent);
		}
		int getTranslationForParent(int index) const
		{
			// TODO
			if (!valid() || index > mMirror->mParents.size())
				throw "";

			return mMirror->mParents[index].translation;
		}

		template<typename Parent, typename Child>
		Parent* caseToImmediateParent(Child* child, const std::string& parent) const
		{
			if (!valid() || !child)
				return nullptr;
			const auto name = getName();
			if (child->mName.namespacedId != getName())
				return nullptr;
			const auto found = std::find_if(mMirror->mParents.begin(), mMirror->mParents.end(), [&](const InternalClassMirror::Entry& e) {
				return e.parent == parent;
				});
			if (found == mMirror->mParents.end())
				return nullptr;
			return reinterpret_cast<Parent*>(found->cast(child));
		}
	public:
		ReflectionInfoHandle(DataMesh* mesh, const std::string& id)
		{
			auto* info = mesh->get(id);
			if (info == nullptr)
			{
				DebugReport("Type %s has no info...\n", id.c_str());
				return;
			}

			mType = info->mType;
			mMirror = info;
			mMesh = mesh;
		}
	private:
		DataMesh* mMesh = nullptr;
		NodeType mType = NodeType::Invalid;
		InternalClassMirror* mMirror = nullptr;
		ReflectionInfoHandle(NodeType type, InternalClassMirror* mirror = nullptr, DataMesh* mesh = nullptr)
			: mType(type), mMirror(mirror), mMesh(mesh)
		{ }
	};
	ReflectionInfoHandle lookupInfo(std::string info)
	{
		return ReflectionInfoHandle(&mDataMesh, info);
	}
	void findParentOfType(std::vector<void*>& out, void* in, const std::string& info, const std::string& key)
	{
		auto hnd = ReflectionInfoHandle(&mDataMesh, info);

		for (int i = 0; i < hnd.getNumParents(); ++i)
		{
			auto parent = hnd.getParent(i);
			if (parent.getName() == key)
				out.push_back(reinterpret_cast<char*>(in) + hnd.getTranslationForParent(i));
			else
				findParentOfType(out, reinterpret_cast<char*>(in) + hnd.getTranslationForParent(i), parent.getName(), key);
		}
	}
};
