#include "fbx.hpp"

#include <fbxsdk.h>
#include <Lib3D/interface/i3dmodel.hpp>
#include <Lib3D/io/fbx2gltf/FbxBlendShapesAccess.hpp>
#include <Lib3D/io/fbx2gltf/FbxLayerElementAccess.hpp>
#include <Lib3D/io/fbx2gltf/FbxSkinningAccess.hpp>
#include <Lib3D/io/fbx2gltf/materials/FbxMaterials.hpp>
#include <Lib3D/io/fbx2gltf/materials/RoughnessMetallicMaterials.hpp>
#include <Lib3D/io/fbx2gltf/materials/TraditionalMaterials.hpp>

#include <Lib3D/io/fbx2gltf/Utils.hpp>
#include <Lib3D/io/fbx2gltf/utils/Image_Utils.hpp>

#include <ThirdParty/glm/gtc/matrix_transform.hpp>
#include <ThirdParty/stb_image.h>

namespace lib3d {

static f32 scaleFactor;

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

using Vec4i = glm::vec<4, u16>;
using Vec4f = glm::vec4;
struct FBXImportContext
{
	px::CollectionHost& model;
	lib3d::Scene& state;
	FbxScene* scene;
	FbxNode* node;
	const s32 parentId;
	const std::string& path;

	std::map<u64, std::size_t>& nodeIdToIndex;
	const std::map<const FbxTexture*, FbxString>& textureLocations;
};
template<typename T>
static glm::vec3 toVec3f(const T& vec)
{
	return { static_cast<const double*>(vec)[0], static_cast<const double*>(vec)[1], static_cast<const double*>(vec)[2] };
}


enum RawTextureUsage {
	RAW_TEXTURE_USAGE_NONE = -1,
	RAW_TEXTURE_USAGE_AMBIENT,
	RAW_TEXTURE_USAGE_DIFFUSE,
	RAW_TEXTURE_USAGE_NORMAL,
	RAW_TEXTURE_USAGE_SPECULAR,
	RAW_TEXTURE_USAGE_SHININESS,
	RAW_TEXTURE_USAGE_EMISSIVE,
	RAW_TEXTURE_USAGE_REFLECTION,
	RAW_TEXTURE_USAGE_ALBEDO,
	RAW_TEXTURE_USAGE_OCCLUSION,
	RAW_TEXTURE_USAGE_ROUGHNESS,
	RAW_TEXTURE_USAGE_METALLIC,
	RAW_TEXTURE_USAGE_MAX
};

enum RawTextureOcclusion { RAW_TEXTURE_OCCLUSION_OPAQUE, RAW_TEXTURE_OCCLUSION_TRANSPARENT };

struct RawTexture {
	std::string name; // logical name in FBX file
	int width;
	int height;
	int mipLevels;
	RawTextureUsage usage;
	RawTextureOcclusion occlusion;
	std::string fileName; // original filename in FBX file
	std::string fileLocation; // inferred path in local filesystem, or ""
};

enum RawMaterialType {
	RAW_MATERIAL_TYPE_OPAQUE,
	RAW_MATERIAL_TYPE_TRANSPARENT,
	RAW_MATERIAL_TYPE_SKINNED_OPAQUE,
	RAW_MATERIAL_TYPE_SKINNED_TRANSPARENT,
};
struct TextureBuilder
{
	struct RawMaterial
	{
		long uuid;
		std::string name;
		RawMaterialType type;
		std::array<int, 8> textures;
	};

	int addMaterial(const long uuid, const std::string& name, RawMaterialType type, const std::array<int, 8>& textures)
	{
		if (name.empty()) {
			return -1;
		}
		for (size_t i = 0; i < materials.size(); i++) {
			if (materials[i].uuid == uuid) return (int)i;

			if (StringUtils::CompareNoCase(materials[i].name, name) == 0 &&
				materials[i].type == type && materials[i].textures == textures) {
				return (int)i;
			}
		}


		materials.push_back(RawMaterial{ uuid, name, type, textures });
		return (int)materials.size() - 1;
	}

	std::vector<RawTexture> textures;
	std::vector<RawMaterial> materials;

	RawTexture getTexture(std::size_t index) const
	{
		return textures[index];
	}
	int addTexture(const std::string& name, const std::string& fileName, const std::string& fileLocation, RawTextureUsage usage)
	{
		if (name.empty()) {
			return -1;
		}
		for (size_t i = 0; i < textures.size(); i++) {
			// we allocate the struct even if the implementing image file is missing
			if (textures[i].usage == usage &&
				StringUtils::CompareNoCase(textures[i].fileLocation, fileLocation) == 0 &&
				StringUtils::CompareNoCase(textures[i].name, name) == 0) {
				return (int)i;
			}
		}

		const ImageUtils::ImageProperties properties = ImageUtils::GetImageProperties(
			!fileLocation.empty() ? fileLocation.c_str() : fileName.c_str());

		RawTexture texture;
		texture.name = name;
		texture.width = properties.width;
		texture.height = properties.height;
		texture.mipLevels =
			(int)ceilf(log2f(std::max((float)properties.width, (float)properties.height)));
		texture.usage = usage;
		texture.occlusion = (properties.occlusion == ImageUtils::IMAGE_TRANSPARENT)
			? RAW_TEXTURE_OCCLUSION_TRANSPARENT
			: RAW_TEXTURE_OCCLUSION_OPAQUE;
		texture.fileName = fileName;
		texture.fileLocation = fileLocation;
		textures.emplace_back(texture);
		return (int)textures.size() - 1;
	}
};

static RawMaterialType GetMaterialType(
	const TextureBuilder& raw,
	const int textures[RAW_TEXTURE_USAGE_MAX],
	const bool vertexTransparency,
	const bool skinned) {
	// DIFFUSE and ALBEDO are different enough to represent distinctly, but they both help determine
	// transparency.
	int diffuseTexture = textures[RAW_TEXTURE_USAGE_DIFFUSE];
	if (diffuseTexture < 0) {
		diffuseTexture = textures[RAW_TEXTURE_USAGE_ALBEDO];
	}
	// determine material type based on texture occlusion.
	if (diffuseTexture >= 0) {
		return (raw.getTexture(diffuseTexture).occlusion == RAW_TEXTURE_OCCLUSION_OPAQUE)
			? (skinned ? RAW_MATERIAL_TYPE_SKINNED_OPAQUE : RAW_MATERIAL_TYPE_OPAQUE)
			: (skinned ? RAW_MATERIAL_TYPE_SKINNED_TRANSPARENT : RAW_MATERIAL_TYPE_TRANSPARENT);
	}

	// else if there is any vertex transparency, treat whole mesh as transparent
	if (vertexTransparency) {
		return skinned ? RAW_MATERIAL_TYPE_SKINNED_TRANSPARENT : RAW_MATERIAL_TYPE_TRANSPARENT;
	}

	// Default to simply opaque.
	return skinned ? RAW_MATERIAL_TYPE_SKINNED_OPAQUE : RAW_MATERIAL_TYPE_OPAQUE;
}
void readNodes(FBXImportContext ctx)
{
	const u64 nodeId = ctx.node->GetUniqueID();
	const char* nodeName = ctx.node->GetName();

	auto bones = ctx.model.getFolder<lib3d::Bone>();

	printf("Node: %s/%s\n", ctx.path.c_str(), nodeName);
	const int nodeIndex = bones->size();
	bones->add();
	auto& node = *bones->at<lib3d::Bone>(nodeIndex);
	// Ignore FBX nodeID
	node.setName(nodeName);
	node.setParent(ctx.parentId);

	ctx.nodeIdToIndex[nodeId] = nodeIndex;

	// TODO: EInheritType

	const auto localTransform = ctx.node->EvaluateLocalTransform();
	const auto localTranslation = localTransform.GetT();
	const auto localRotation = localTransform.GetQ();
	const auto localScaling = computeLocalScale(ctx.node);

	scaleFactor = FbxSystemUnit::m.GetConversionFactorFrom(FbxSystemUnit::cm);

	node.setSRT(lib3d::SRT3{
		toVec3f(localScaling), toVec3f(localRotation.DecomposeSphericalXYZ()),
		toVec3f(localTranslation) * scaleFactor });

	if (ctx.parentId >= 0)
		bones->at<lib3d::Bone>(ctx.parentId)->addChild(nodeIndex);
	
	// TODO: Non zero root

	for (int child = 0; child < ctx.node->GetChildCount(); ++child)
		readNodes(FBXImportContext{ ctx.model, ctx.state, ctx.scene, ctx.node->GetChild(child), nodeIndex, ctx.path + "/" + nodeName,
			ctx.nodeIdToIndex, ctx.textureLocations});
}
void readMesh(FBXImportContext ctx, TextureBuilder& raw)
{
	auto bones = ctx.model.getFolder<lib3d::Bone>();
	auto polygons = ctx.model.getFolder<lib3d::Polygon>();

	FbxGeometryConverter meshConverter(ctx.scene->GetFbxManager());
	meshConverter.Triangulate(ctx.node->GetNodeAttribute(), true);
	meshConverter.SplitMeshesPerMaterial(ctx.scene, true);
	FbxMesh* pMesh = ctx.node->GetMesh();

	const auto surfaceId = pMesh->GetUniqueID();

	int nodeId = ctx.nodeIdToIndex[ctx.node->GetUniqueID()];
	
	// TODO: Don't load duplicate surfaces (use unique ids)

	const char* meshName = (ctx.node->GetName()[0] != '\0') ? ctx.node->GetName() : pMesh->GetName();
	const int rawSurfaceIndex = polygons->size();
	polygons->add();
	polygons->at<lib3d::Polygon>(rawSurfaceIndex)->setName(meshName);
	// Ignores surface id

	const FbxVector4* controlPoints = pMesh->GetControlPoints();
	const FbxLayerElementAccess<FbxVector4> normalLayer(
		pMesh->GetElementNormal(), pMesh->GetElementNormalCount());
	const FbxLayerElementAccess<FbxVector4> binormalLayer(
		pMesh->GetElementBinormal(), pMesh->GetElementBinormalCount());
	const FbxLayerElementAccess<FbxVector4> tangentLayer(
		pMesh->GetElementTangent(), pMesh->GetElementTangentCount());
	const FbxLayerElementAccess<FbxColor> colorLayer(
		pMesh->GetElementVertexColor(), pMesh->GetElementVertexColorCount());
	const FbxLayerElementAccess<FbxVector2> uvLayer0(
		pMesh->GetElementUV(0), pMesh->GetElementUVCount());
	const FbxLayerElementAccess<FbxVector2> uvLayer1(
		pMesh->GetElementUV(1), pMesh->GetElementUVCount());
	const FbxSkinningAccess skinning(pMesh, ctx.scene, ctx.node);
	const FbxMaterialsAccess materials(pMesh, ctx.textureLocations);
	const FbxBlendShapesAccess blendShapes(pMesh);

	printf(
		"mesh %d: %s (skinned: %s)\n",
		rawSurfaceIndex,
		meshName,
		skinning.IsSkinned() ? bones->at<lib3d::Bone>(ctx.nodeIdToIndex.at(skinning.GetRootNode()))->getName().c_str()
		: "NO");
	const FbxVector4 meshTranslation = ctx.node->GetGeometricTranslation(FbxNode::eSourcePivot);
	const FbxVector4 meshRotation = ctx.node->GetGeometricRotation(FbxNode::eSourcePivot);
	const FbxVector4 meshScaling = ctx.node->GetGeometricScaling(FbxNode::eSourcePivot);
	const FbxAMatrix meshTransform(meshTranslation, meshRotation, meshScaling);
	const FbxMatrix transform = meshTransform;

	// Remove translation & scaling from transforms that will bi applied to normals, tangents &
	// binormals
	const FbxMatrix normalTransform(FbxVector4(), meshRotation, meshScaling);
	const FbxMatrix inverseTransposeTransform = normalTransform.Inverse().Transpose();

	auto& rawSurface = *polygons->at<lib3d::Polygon>(rawSurfaceIndex);

	rawSurface.setAttrib(lib3d::Polygon::SimpleAttrib::Position, true);
	if (normalLayer.LayerPresent())
		rawSurface.setAttrib(lib3d::Polygon::SimpleAttrib::Normal, true);
	
	// TODO: Tangent, Binormal -> NBT
	if (colorLayer.LayerPresent())
		rawSurface.setAttrib(lib3d::Polygon::SimpleAttrib::Color0, true);
	// TODO: Multiple UV layers
	if (uvLayer0.LayerPresent())
		rawSurface.setAttrib(lib3d::Polygon::SimpleAttrib::TexCoord0, true);
	//	if (uvLayer1.LayerPresent())
	//		rawSurface.setAttrib(lib3d::Polygon::SimpleAttrib::TexCoord1, true);
	// TODO: skinning
	assert(!skinning.IsSkinned());
	
	
	glm::mat4 scaleMatrix { 1.0f };
	glm::scale(scaleMatrix, glm::vec3(scaleFactor, scaleFactor, scaleFactor));
	glm::mat4 invScaleMatrix = glm::inverse(scaleMatrix);

#if 0
	rawSurface.skeletonRootId =
		(skinning.IsSkinned()) ? skinning.GetRootNode() : pNode->GetUniqueID();
	for (int jointIndex = 0; jointIndex < skinning.GetNodeCount(); jointIndex++) {
		const long jointId = skinning.GetJointId(jointIndex);
		raw.GetNode(raw.GetNodeById(jointId)).isJoint = true;

		rawSurface.jointIds.emplace_back(jointId);
		rawSurface.inverseBindMatrices.push_back(
			invScaleMatrix * toMat4f(skinning.GetInverseBindMatrix(jointIndex)) * scaleMatrix);
		rawSurface.jointGeometryMins.emplace_back(FLT_MAX, FLT_MAX, FLT_MAX);
		rawSurface.jointGeometryMaxs.emplace_back(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	}
#endif

	// TODO: Blend shapes

	// We already split mesh by material:
	const std::shared_ptr<FbxMaterialInfo> fbxMaterial = materials.GetMaterial(0);
	const std::vector<std::string> userProperties = materials.GetUserProperties(0);

	std::array<int, 8> textures;
	std::fill_n(textures.data(), 8, -1);

	FbxString materialName;
	long materialId;

	if (fbxMaterial == nullptr) {
		assert(0); // TODO: Default material
	}
	else
	{
		FbxTraditionalMaterialInfo* fbxMatInfo =
			static_cast<FbxTraditionalMaterialInfo*>(fbxMaterial.get());
		materialName = fbxMaterial->name;
		materialId = fbxMaterial->id;

		const auto maybeAddTexture = [&](const FbxFileTexture* tex, RawTextureUsage usage) {
			if (tex != nullptr) {
				// dig out the inferred filename from the textureLocations map
				FbxString inferredPath = ctx.textureLocations.find(tex)->second;
				textures[usage] =
					raw.addTexture(tex->GetName(), tex->GetFileName(), inferredPath.Buffer(), usage);
			}
		};

		maybeAddTexture(fbxMatInfo->texDiffuse, RAW_TEXTURE_USAGE_DIFFUSE);
		maybeAddTexture(fbxMatInfo->texNormal, RAW_TEXTURE_USAGE_NORMAL);
		maybeAddTexture(fbxMatInfo->texEmissive, RAW_TEXTURE_USAGE_EMISSIVE);
		maybeAddTexture(fbxMatInfo->texShininess, RAW_TEXTURE_USAGE_SHININESS);
		maybeAddTexture(fbxMatInfo->texAmbient, RAW_TEXTURE_USAGE_AMBIENT);
		maybeAddTexture(fbxMatInfo->texSpecular, RAW_TEXTURE_USAGE_SPECULAR);
	}

	bool vertexTransparency = false;

	int polygonVertexIndex = 0;
	for (int polygonIndex = 0; polygonIndex < pMesh->GetPolygonCount(); polygonIndex++) {
		FBX_ASSERT(pMesh->GetPolygonSize(polygonIndex) == 3);

		std::array<Polygon::SimpleVertex, 3> rawVertices;
		for (int vertexIndex = 0; vertexIndex < 3; vertexIndex++, polygonVertexIndex++) {
			const int controlPointIndex = pMesh->GetPolygonVertex(polygonIndex, vertexIndex);

			// Note that the default values here must be the same as the RawVertex default values!
			const FbxVector4 fbxPosition = transform.MultNormalize(controlPoints[controlPointIndex]);
			const FbxVector4 fbxNormal = normalLayer.GetElement(
				polygonIndex,
				polygonVertexIndex,
				controlPointIndex,
				FbxVector4(0.0f, 0.0f, 0.0f, 0.0f),
				inverseTransposeTransform,
				true);
			const FbxVector4 fbxTangent = tangentLayer.GetElement(
				polygonIndex,
				polygonVertexIndex,
				controlPointIndex,
				FbxVector4(0.0f, 0.0f, 0.0f, 0.0f),
				inverseTransposeTransform,
				true);
			const FbxVector4 fbxBinormal = binormalLayer.GetElement(
				polygonIndex,
				polygonVertexIndex,
				controlPointIndex,
				FbxVector4(0.0f, 0.0f, 0.0f, 0.0f),
				inverseTransposeTransform,
				true);
			const FbxColor fbxColor = colorLayer.GetElement(
				polygonIndex, polygonVertexIndex, controlPointIndex, FbxColor(0.0f, 0.0f, 0.0f, 0.0f));
			const FbxVector2 fbxUV0 = uvLayer0.GetElement(
				polygonIndex, polygonVertexIndex, controlPointIndex, FbxVector2(0.0f, 0.0f));
			const FbxVector2 fbxUV1 = uvLayer1.GetElement(
				polygonIndex, polygonVertexIndex, controlPointIndex, FbxVector2(0.0f, 0.0f));

			Polygon::SimpleVertex& vertex = rawVertices[vertexIndex];
			vertex.position[0] = (float)fbxPosition[0] * scaleFactor;
			vertex.position[1] = (float)fbxPosition[1] * scaleFactor;
			vertex.position[2] = (float)fbxPosition[2] * scaleFactor;
			vertex.normal[0] = (float)fbxNormal[0];
			vertex.normal[1] = (float)fbxNormal[1];
			vertex.normal[2] = (float)fbxNormal[2];
			//	vertex.tangent[0] = (float)fbxTangent[0];
			//	vertex.tangent[1] = (float)fbxTangent[1];
			//	vertex.tangent[2] = (float)fbxTangent[2];
			//	vertex.tangent[3] = (float)fbxTangent[3];
			//	vertex.binormal[0] = (float)fbxBinormal[0];
			//	vertex.binormal[1] = (float)fbxBinormal[1];
			//	vertex.binormal[2] = (float)fbxBinormal[2];
			vertex.colors[0][0] = (float)fbxColor.mRed / 255.0f; // TODO: Round
			vertex.colors[0][1] = (float)fbxColor.mGreen / 255.0f;
			vertex.colors[0][2] = (float)fbxColor.mBlue / 255.0f;
			vertex.colors[0][3] = (float)fbxColor.mAlpha / 255.0f;
			vertex.uvs[0][0] = (float)fbxUV0[0];
			vertex.uvs[0][1] = (float)fbxUV0[1];
			vertex.uvs[1][0] = (float)fbxUV1[0];
			vertex.uvs[1][1] = (float)fbxUV1[1];
			//	vertex.jointIndices = skinning.GetVertexIndices(controlPointIndex);
			//	vertex.jointWeights = skinning.GetVertexWeights(controlPointIndex);
			//	vertex.polarityUv0 = false;

			// flag this triangle as transparent if any of its corner vertices substantially deviates from
			// fully opaque
			vertexTransparency |= colorLayer.LayerPresent() && (fabs(fbxColor.mAlpha - 1.0) > 1e-3);

			// TODO: Blend shapes
#if 0
			if (skinning.IsSkinned()) {
				const int jointIndices[FbxSkinningAccess::MAX_WEIGHTS] = { vertex.jointIndices[0],
																		  vertex.jointIndices[1],
																		  vertex.jointIndices[2],
																		  vertex.jointIndices[3] };
				const float jointWeights[FbxSkinningAccess::MAX_WEIGHTS] = { vertex.jointWeights[0],
																			vertex.jointWeights[1],
																			vertex.jointWeights[2],
																			vertex.jointWeights[3] };
				const FbxMatrix skinningMatrix =
					skinning.GetJointSkinningTransform(jointIndices[0]) * jointWeights[0] +
					skinning.GetJointSkinningTransform(jointIndices[1]) * jointWeights[1] +
					skinning.GetJointSkinningTransform(jointIndices[2]) * jointWeights[2] +
					skinning.GetJointSkinningTransform(jointIndices[3]) * jointWeights[3];

				const FbxVector4 globalPosition = skinningMatrix.MultNormalize(fbxPosition);
				for (int i = 0; i < FbxSkinningAccess::MAX_WEIGHTS; i++) {
					if (jointWeights[i] > 0.0f) {
						const FbxVector4 localPosition =
							skinning.GetJointInverseGlobalTransforms(jointIndices[i])
							.MultNormalize(globalPosition);

						Vec3f& mins = rawSurface.jointGeometryMins[jointIndices[i]];
						mins[0] = std::min(mins[0], (float)localPosition[0]);
						mins[1] = std::min(mins[1], (float)localPosition[1]);
						mins[2] = std::min(mins[2], (float)localPosition[2]);

						Vec3f& maxs = rawSurface.jointGeometryMaxs[jointIndices[i]];
						maxs[0] = std::max(maxs[0], (float)localPosition[0]);
						maxs[1] = std::max(maxs[1], (float)localPosition[1]);
						maxs[2] = std::max(maxs[2], (float)localPosition[2]);
					}
				}
			}
		}
#endif
#if 0
		if (textures[0] != -1) {
			// Distinguish vertices that are used by triangles with a different texture polarity to avoid
			// degenerate tangent space smoothing.
			const bool polarity =
				TriangleTexturePolarity(rawVertices[0].uv0, rawVertices[1].uv0, rawVertices[2].uv0);
			rawVertices[0].polarityUv0 = polarity;
			rawVertices[1].polarityUv0 = polarity;
			rawVertices[2].polarityUv0 = polarity;
		}
#endif
		
		}
		rawSurface.addTriangle(rawVertices);
	}


	const RawMaterialType materialType =
		GetMaterialType(raw, textures.data(), vertexTransparency, skinning.IsSkinned());
	const int rawMaterialIndex = raw.addMaterial(materialId, materialName.Buffer(), materialType, textures);


	if (nodeId >= 0)
		bones->at<lib3d::Bone>(nodeId)->addDisplay({ (u32)rawMaterialIndex, (u32)rawSurfaceIndex, 0 });

}
void readNodeAttributes(FBXImportContext ctx, TextureBuilder& builder)
{
	if (!ctx.node->GetVisibility()) return;

	FbxNodeAttribute* pNodeAttribute = ctx.node->GetNodeAttribute();
	if (pNodeAttribute)
	{
		const FbxNodeAttribute::EType attributeType = pNodeAttribute->GetAttributeType();
		switch (attributeType) {
		case FbxNodeAttribute::eMesh:
		case FbxNodeAttribute::eNurbs:
		case FbxNodeAttribute::eNurbsSurface:
		case FbxNodeAttribute::eTrimNurbsSurface:
		case FbxNodeAttribute::ePatch:
			readMesh(ctx, builder);
			break;
		case FbxNodeAttribute::eCamera:
		case FbxNodeAttribute::eLight:
		case FbxNodeAttribute::eUnknown:
		case FbxNodeAttribute::eNull:
		case FbxNodeAttribute::eMarker:
		case FbxNodeAttribute::eSkeleton:
		case FbxNodeAttribute::eCameraStereo:
		case FbxNodeAttribute::eCameraSwitcher:
		case FbxNodeAttribute::eOpticalReference:
		case FbxNodeAttribute::eOpticalMarker:
		case FbxNodeAttribute::eNurbsCurve:
		case FbxNodeAttribute::eBoundary:
		case FbxNodeAttribute::eShape:
		case FbxNodeAttribute::eLODGroup:
		case FbxNodeAttribute::eSubDiv:
		case FbxNodeAttribute::eCachedEffect:
		case FbxNodeAttribute::eLine:
			break;
		}
	}



	for (int child = 0; child < ctx.node->GetChildCount(); ++child)
		readNodeAttributes(FBXImportContext{ ctx.model, ctx.state, ctx.scene, ctx.node->GetChild(child),
			(s32)ctx.nodeIdToIndex.at(ctx.node->GetUniqueID()), "",  ctx.nodeIdToIndex, ctx.textureLocations }, builder );
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

	std::map<const FbxTexture*, FbxString> textureLocations;
	FindFbxTextures(scene.get(), fbxFileName, { "png", "jpg", "jpeg" }, textureLocations);


	FbxAxisSystem::MayaYUp.ConvertScene(scene.get());

	FbxSystemUnit unit = scene->GetGlobalSettings().GetSystemUnit();
	if (unit != FbxSystemUnit::cm)
		FbxSystemUnit::cm.ConvertScene(scene.get());

	lib3d::Scene* p3dScene = dynamic_cast<lib3d::Scene*>(state.mOwner.get());
	assert(p3dScene);
	lib3d::Scene& _state = *p3dScene;

	auto models = _state.getFolder<px::CollectionHost>();
	models->add();
	models->at<px::CollectionHost>(0)->mpScene = p3dScene;
	std::map<u64, std::size_t> nodeIdToIndex;
	FBXImportContext ctx{ *models->at<px::CollectionHost>(models->size() - 1),
		_state, scene.get(), scene->GetRootNode(), -1, "", nodeIdToIndex, textureLocations };
	readNodes(ctx);

	TextureBuilder builder;

	readNodeAttributes(ctx, builder);
	//	ReadAnimations(raw, pScene, options);

	// Generate textures
	auto textures = _state.getFolder<lib3d::Texture>();
	for (const auto& texture : builder.textures)
	{
		textures->add();
		lib3d::Texture* tex = textures->at<lib3d::Texture>(textures->size()-1);
		tex->setName(texture.name);
		tex->setWidth(texture.width);
		tex->setHeight(texture.height);
		// tex->setMipLevel(texture.mipLevels);
		// usage, occlusion
		tex->setEncoder(true, true, lib3d::Texture::Occlusion::Opaque);

		int width, height, channels;
		u8* image = stbi_load(texture.fileLocation.c_str(),
			&width,
			&height,
			&channels,
			STBI_rgb_alpha);

		tex->encode(image);
		
		stbi_image_free(image);
	}
	auto& model = *models->at<px::CollectionHost>(models->size() - 1);
	auto materials = model.getFolder<lib3d::Material>();
	for (const auto& material : builder.materials)
	{
		materials->add();
		lib3d::Material* mat = materials->at<lib3d::Material>(materials->size() - 1);
		mat->setName(material.name);

		std::vector<std::string> texs;
		for (int it : material.textures)
			if (it >= 0)
				texs.push_back(builder.textures[it].name);

		mat->configure(lib3d::PixelOcclusion::Opaque, texs);
	}
}
void FbxIo::write(px::Dynamic& state, oishii::v2::Writer& writer)
{

}


} // namespace lib3d
