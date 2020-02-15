#include "fbx.hpp"

#include <fbxsdk.h>
#include <Lib3D/interface/i3dmodel.hpp>

namespace lib3d {

template<typename T>
struct FBXDestroyer
{
	void operator()(T* p)
	{
		if (p) p->Destroy();
	}
};

template<typename T>
using FBXPtr = std::unique_ptr<T, FBXDestroyer<T>>;

template<typename T, typename... Args>
FBXPtr<T> make_fbx(FbxManager* pMgr, Args... args)
{
	return FBXPtr<T>(T::Create(pMgr, args...));
}

// https://github.com/facebookincubator/FBX2glTF/blob/3c081695108e5793a58984c8ca751477921e44eb/src/fbx/Fbx2Raw.cpp#L37
static std::string NativeToUTF8(const std::string& str) {
#if _WIN32
	char* u8cstr = nullptr;
#if 0 // (_UNICODE || UNICODE)
	FbxWCToUTF8(reinterpret_cast<const char*>(str.c_str()), u8cstr);
#else
	FbxAnsiToUTF8(str.c_str(), u8cstr);
#endif
	if (!u8cstr) {
		return str;
	}
	else {
		std::string u8str = u8cstr;
		// delete[] u8cstr;
		return u8str;
	}
#else
	return str;
#endif
}

struct FBXImportContext
{
	lib3d::Scene& state;
	FbxScene* scene;
	FbxNode* node;
	const u32 parentId;
	const std::string& path;
};
void readNodes(FBXImportContext ctx)
{
	const u64 nodeId = ctx.node->GetUniqueID();
	const char* nodeName = ctx.node->GetName();

	printf("Node: %s\n", nodeName);
	//

	//const int nodeIndex = ctx.state.AddNode(nodeId, nodeName, ctx.parentId);

}
void FbxIo::read(px::Dynamic& state, oishii::BinaryReader& reader)
{
	const std::string fbxFileName = NativeToUTF8(reader.getFile());

	FBXPtr<FbxManager> pManager(FbxManager::Create());

	pManager->SetIOSettings(FbxIOSettings::Create(pManager.get(), IOSROOT));

	FBXPtr<FbxScene> scene(nullptr);

	{
		auto pImporter = make_fbx<FbxImporter>(pManager.get(), "");

		if (!pImporter->Initialize(fbxFileName.c_str(), -1, pManager->GetIOSettings()))
		{
			printf("%s\n", pImporter->GetStatus().GetErrorString());

			return;
		}

		scene = make_fbx<FbxScene>(pManager.get(), "fbxScene");
		pImporter->Import(scene.get());
	}

	if (!scene)
		return;

	// TODO: Textures
	FbxAxisSystem::MayaYUp.ConvertScene(scene.get());

	FbxSystemUnit unit = scene->GetGlobalSettings().GetSystemUnit();
	if (unit != FbxSystemUnit::cm)
		FbxSystemUnit::cm.ConvertScene(scene.get());

	lib3d::Scene* p3dScene = dynamic_cast<lib3d::Scene*>(state.mOwner.get());
	assert(p3dScene);
	lib3d::Scene& _state = *p3dScene;


	readNodes({ _state, scene.get(), scene->GetRootNode(), 0, "" });
	//	ReadNodeAttributes(raw, pScene, pScene->GetRootNode(), textureLocations);
	//	ReadAnimations(raw, pScene, options);

	return;
}
void FbxIo::write(px::Dynamic& state, oishii::v2::Writer& writer)
{

}


} // namespace lib3d
