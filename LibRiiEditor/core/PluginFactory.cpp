#include "PluginFactory.hpp"
#include <LibRiiEditor/common.hpp>

bool PluginFactory::registerPlugin(const pl::Package& package)
{
	if (package.mEditors.empty() && package.mImporters.empty() && package.mExporters.empty() && package.mInterfaces.empty())
	{
		DebugReport("Plugin is empty.\n");
		return true;
	}

	// TODO: Map the ids

	for (const auto ptr : package.mEditors)
	{
		mPlugins.emplace_back(ptr->clone());
		mDataMesh.declare(ptr->mId.namespacedId, NodeType::FileState, ptr->mId);
		for (const auto& m : ptr->mMirror)
			mDataMesh.enqueueHierarchy(m);
	}
	for (const auto ptr : package.mImporters)
	{
		mImporters.emplace_back(ptr->clone());
	}
	for (const auto ptr : package.mExporters)
		mExporters.emplace_back(ptr->clone());

	for (const auto ptr : package.mInterfaces)
	{
		mDataMesh.declare(ptr->mName.namespacedId, NodeType::FileState, ptr->mName, ptr->mFlag);
		for (const auto& m : ptr->mMirror)
			mDataMesh.enqueueHierarchy(m);
	}
	return true;
}

std::optional<PluginFactory::SpawnedImporter> PluginFactory::spawnImporter(const std::string& fileName, oishii::BinaryReader& reader)
{
	using MatchResult = pl::ImporterSpawner::MatchResult;

	std::map<std::size_t, std::pair<MatchResult, std::string>> matched;

	// Unfortunate linear time search
	std::size_t i = 0;
	for (const auto& plugin : mImporters)
	{
		ScopedInc incrementor(i);
		oishii::JumpOut reader_guard(reader, reader.tell());

		const auto match = plugin->match(fileName, reader);

		if (match.first != MatchResult::Mismatch)
			matched[i] = match;
	}

	if (matched.size() == 0)
	{
		DebugReport("No matches.\n");
		return {};
	}
	else if (matched.size() > 1)
	{
		// FIXME: Find best match or prompt user
		DebugReport("Multiple matches.\n");
		return {};
	}
	else
	{
		DebugReport("Success spawning importer\n");
		return std::optional<PluginFactory::SpawnedImporter> {
			SpawnedImporter {
				matched.begin()->second.second,
				mImporters[matched.begin()->first]->spawn()
			}
		};
	}
}

std::unique_ptr<pl::FileState> PluginFactory::spawnFileState(const std::string& fileStateId)
{
	for (const auto& it : mPlugins)
	{
		if (it->mId.namespacedId == fileStateId)
		{
			auto spawned = it->spawn();
			spawned->mName = it->mId;
			return spawned;
		}
	}

	return nullptr;
}

void PluginFactory::DataMesh::compute()
{
	while (!mToInsert.empty())
	{
		const auto& cmd = mToInsert.front();

		// Always ensure the data mesh is in sync with the rest
		if (mClasses.find(std::string(cmd.base)) == mClasses.end())
		{
			printf("Warning: Type %s references undefined parent %s.\n", cmd.derived.data(), cmd.base.data());
		}
		else
		{
			mClasses[std::string(cmd.base)].derived = cmd.base;
			mClasses[std::string(cmd.base)].mChildren.push_back(std::string(cmd.derived));
		}
		// TODO
		mClasses[std::string(cmd.derived)].derived = cmd.derived;

		mClasses[std::string(cmd.derived)].mParents.push_back({ std::string(cmd.base), cmd.translation });
		mToInsert.pop();
	}
}

void PluginFactory::computeDataMesh()
{
	mDataMesh.compute();
}
static pl::RichName xf_name = {
	
};
PluginFactory::PluginFactory()
{
	mDataMesh.declare("transform_stack", NodeType::Interface, {
		"Transform Stack",
		"transform_stack",
		"transform"
	});
}
