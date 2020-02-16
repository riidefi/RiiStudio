#pragma once

#include <Lib3D/io/fbx2gltf/utils/File_Utils.hpp>
#include <Lib3D/io/fbx2gltf/utils/String_Utils.hpp>

// https://github.com/facebookincubator/FBX2glTF/blob/3c081695108e5793a58984c8ca751477921e44eb/src/fbx/Fbx2Raw.cpp#L1014
static std::string FindFileLoosely(
	const std::string& fbxFileName,
	const std::string& directory,
	const std::vector<std::string>& directoryFileList) {
	if (FileUtils::FileExists(fbxFileName)) {
		return fbxFileName;
	}

	// From e.g. C:/Assets/Texture.jpg, extract 'Texture.jpg'
	const std::string fileName = FileUtils::GetFileName(fbxFileName);

	// Try to find a match with extension.
	for (const auto& file : directoryFileList) {
		if (StringUtils::CompareNoCase(fileName, FileUtils::GetFileName(file)) == 0) {
			return directory + "/" + file;
		}
	}

	// Get the file name without file extension.
	const std::string fileBase = FileUtils::GetFileBase(fileName);

	// Try to find a match that ignores file extension
	for (const auto& file : directoryFileList) {
		if (StringUtils::CompareNoCase(fileBase, FileUtils::GetFileBase(file)) == 0) {
			return directory + "/" + file;
		}
	}

	return "";
}
/**
 * Try to locate the best match to the given texture filename, as provided in the FBX,
 * possibly searching through the provided folders for a reasonable-looking match.
 *
 * Returns empty string if no match can be found, else the absolute path of the file.
 **/
static std::string FindFbxTexture(
	const std::string& textureFileName,
	const std::vector<std::string>& folders,
	const std::vector<std::vector<std::string>>& folderContents) {
	// it might exist exactly as-is on the running machine's filesystem
	if (FileUtils::FileExists(textureFileName)) {
		return textureFileName;
	}
	// else look in other designated folders
	for (int ii = 0; ii < folders.size(); ii++) {
		const auto& fileLocation = FindFileLoosely(textureFileName, folders[ii], folderContents[ii]);
		if (!fileLocation.empty()) {
			return FileUtils::GetAbsolutePath(fileLocation);
		}
	}
	return "";
}

/*
	The texture file names inside of the FBX often contain some long author-specific
	path with the wrong extensions. For instance, all of the art assets may be PSD
	files in the FBX metadata, but in practice they are delivered as TGA or PNG files.
	This function takes a texture file name stored in the FBX, which may be an absolute
	path on the author's computer such as "C:\MyProject\TextureName.psd", and matches
	it to a list of existing texture files in the same directory as the FBX file.
*/
static void FindFbxTextures(
	FbxScene* pScene,
	const std::string& fbxFileName,
	const std::set<std::string>& extensions,
	std::map<const FbxTexture*, FbxString>& textureLocations) {
	// figure out what folder the FBX file is in,
	const auto& fbxFolder = FileUtils::getFolder(fbxFileName);
	std::vector<std::string> folders{
		// first search filename.fbm folder which the SDK itself expands embedded textures into,
		fbxFolder + "/" + FileUtils::GetFileBase(fbxFileName) + ".fbm", // filename.fbm
		// then the FBX folder itself,
		fbxFolder,
		// then finally our working directory
		FileUtils::GetCurrentFolder(),
	};

	// List the contents of each of these folders (if they exist)
	std::vector<std::vector<std::string>> folderContents;
	for (const auto& folder : folders) {
		if (FileUtils::FolderExists(folder)) {
			folderContents.push_back(FileUtils::ListFolderFiles(folder, extensions));
		}
		else {
			folderContents.push_back({});
		}
	}

	// Try to match the FBX texture names with the actual files on disk.
	for (int i = 0; i < pScene->GetTextureCount(); i++) {
		const FbxFileTexture* pFileTexture = FbxCast<FbxFileTexture>(pScene->GetTexture(i));
		if (pFileTexture != nullptr) {
			const std::string fileLocation =
				FindFbxTexture(pFileTexture->GetFileName(), folders, folderContents);
			// always extend the mapping (even for files we didn't find)
			textureLocations.emplace(pFileTexture, fileLocation.c_str());
			if (fileLocation.empty()) {
				printf(
					"Warning: could not find a image file for texture: %s.\n", pFileTexture->GetName());
			}
			//else if (verboseOutput) {
				printf("Found texture '%s' at: %s\n", pFileTexture->GetName(), fileLocation.c_str());
			//}
		}
	}
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
static FbxVector4 computeLocalScale(FbxNode* pNode, FbxTime pTime = FBXSDK_TIME_INFINITE) {
	const FbxVector4 lScale = pNode->EvaluateLocalTransform(pTime).GetS();

	if (pNode->GetParent() == nullptr || pNode->GetTransform().GetInheritType() != FbxTransform::eInheritRrs)
		return lScale;

	return FbxVector4(1, 1, 1, 1);
}
