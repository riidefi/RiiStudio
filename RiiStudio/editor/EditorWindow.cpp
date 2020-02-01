#include "EditorWindow.hpp"
#include <LibCore/api/Collection.hpp>

struct GenericCollectionOutliner : public IStudioWindow
{
	GenericCollectionOutliner(px::CollectionHost& host) : IStudioWindow("Outliner"), mHost(host)
	{}

	void draw() noexcept override
	{
		for (u32 i = 0; i < mHost.getNumFolders(); ++i)
		{
			auto hnd = mHost.getFolder(i);

			ImGui::Text(hnd.getType().c_str());

			for (int j = 0; j < hnd.size(); ++j)
			{
				ImGui::Text("Item %u", i);
				auto subHnds = px::ReflectionMesh::getInstance()->
					findParentOfType<px::CollectionHost>(hnd.atDynamic(j));
				if (subHnds.size() == 1)
				{
					for (u32 k = 0; k < subHnds[0]->getNumFolders(); ++k)
					{
						ImGui::Text(subHnds[0]->getFolder(k).getType().c_str());
					}
				}

			}

	
		}
	}

private:
	px::CollectionHost& mHost;
};

struct WIdek : public IStudioWindow
{
	WIdek() : IStudioWindow("Idek", false)
	{}
};

EditorWindow::EditorWindow(px::Dynamic state, const std::string& path)
	: mState({ std::move(state.mOwner), state.mBase, state.mType }), mFilePath(path), IStudioWindow(path, true)
{
#if 0
	// TODO: Recursive..
	const auto hnd = px::ReflectionMesh::getInstance()->lookupInfo(
		mState.mType);
	assert(hnd);

	for (int i = 0; i < hnd.getNumParents(); ++i)
	{
		if (hnd.getParent(i).getName() == px::CollectionHost::TypeInfo.namespacedId)
		{
			px::CollectionHost* pHost = hnd.castToImmediateParent<px::CollectionHost>(mState.mBase, i);
			assert(pHost);
			if (!pHost) return;

		}
	}
#endif
	std::vector<px::CollectionHost*> collectionhosts= px::ReflectionMesh::getInstance()->findParentOfType<px::CollectionHost>(mState);

	if (collectionhosts.size() > 1)
	{
		printf("Cannot spawn editor window; too many collection hosts.\n");
	}
	else if (collectionhosts.empty())
	{
		printf("State has no collection hosts.\n");
	}
	else
	{
		assert(collectionhosts[0]);

		px::CollectionHost& host = *collectionhosts[0];

		attachWindow(std::make_unique<GenericCollectionOutliner>(host));
		attachWindow(std::make_unique<WIdek>());
	}
}
void EditorWindow::draw() noexcept
{

}
