/**
 * Copyright (c) Facebook, Inc. and its affiliates.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "File_Utils.hpp"

#include <fstream>
#include <set>
#include <string>
#include <vector>

#include <stdint.h>
#include <stdio.h>

#include <filesystem>

#include "String_Utils.hpp"

namespace FileUtils {

std::vector<std::string> ListFolderFiles(
    std::string folder,
    const std::set<std::string>& matchExtensions) {
  std::vector<std::string> fileList;
  if (folder.empty()) {
    folder = ".";
  }
  for (const auto& entry : std::filesystem::directory_iterator(folder)) {
    const auto& suffix = FileUtils::GetFileSuffix(entry.path().string());
    if (suffix.has_value()) {
      const auto& suffix_str = StringUtils::ToLower(suffix.value());
      if (matchExtensions.find(suffix_str) != matchExtensions.end()) {
        fileList.push_back(entry.path().filename().string());
      }
    }
  }
  return fileList;
}

bool CreatePath(const std::string path) {
  const auto& parent = std::filesystem::path(path).parent_path();
  if (parent.empty()) {
    // this is either CWD or std::filesystem root; either way it exists
    return true;
  }
  if (std::filesystem::exists(parent)) {
    return std::filesystem::is_directory(parent);
  }
  return std::filesystem::create_directory(parent);
}

bool CopyFile(const std::string& srcFilename, const std::string& dstFilename, bool createPath) {
  std::ifstream srcFile(srcFilename, std::ios::binary);
  if (!srcFile) {
    printf("Warning: Couldn't open file %s for reading.\n", srcFilename.c_str());
    return false;
  }
  // find source file length
  srcFile.seekg(0, std::ios::end);
  std::streamsize srcSize = srcFile.tellg();
  srcFile.seekg(0, std::ios::beg);

  if (createPath && !CreatePath(dstFilename.c_str())) {
    printf("Warning: Couldn't create directory %s.\n", dstFilename.c_str());
    return false;
  }

  std::ofstream dstFile(dstFilename, std::ios::binary | std::ios::trunc);
  if (!dstFile) {
    printf("Warning: Couldn't open file %s for writing.\n", dstFilename.c_str());
    return false;
  }
  dstFile << srcFile.rdbuf();
  std::streamsize dstSize = dstFile.tellp();
  if (srcSize == dstSize) {
    return true;
  }
  printf(
      "Warning: Only copied %lu bytes to %s, when %s is %lu bytes long.\n",
      dstSize,
      dstFilename.c_str(),
      srcFilename.c_str(),
      srcSize);
  return false;
}
} // namespace FileUtils
