#include "librii/jparticle/Sections/TextureBlock.hpp"
#include "rsl/FsDialog.hpp"
#include "BTIUtils.hpp"

#include <filesystem>

  Result<void> librii::jpa::importBTI(std::vector<librii::jpa::TextureBlock>& tex) {
  auto default_filename = std::filesystem::path("").filename();
  std::vector<std::string> filters{"??? (*.bti)", "*.bti"};
  auto results =
      rsl::ReadOneFile("Open File"_j, default_filename.string(), filters);
  if (!results) {
    rsl::ErrorDialog("No file selected");
    return {};
  }

  rsl::trace("Attempting to load from {}", results->path.string());

  librii::j3d::Tex tmp;
  auto reader = TRY(oishii::BinaryReader::FromFilePath(results->path.string(),
                                                       std::endian::big));
  rsl::SafeReader safeReader(reader);
  TRY(tmp.transfer(safeReader));
  auto buffer_addr = tmp.ofsTex;
  auto buffer_size = librii::gx::computeImageSize(
      tmp.mWidth, tmp.mHeight, tmp.mFormat, tmp.mMipmapLevel);

  auto image_data = TRY(reader.tryReadBuffer<u8>(buffer_size, buffer_addr));

  librii::jpa::TextureBlock data = librii::jpa::TextureBlock(tmp, image_data);
  // Strip out file extension
  data.setName(
      std::string(results->path.replace_extension().filename().string()));
  tex.push_back(data);
  return {};
}

Result<void> librii::jpa::replaceBTI(const char* btiName,
                                     librii::jpa::TextureBlock& data) {
  auto default_filename = std::filesystem::path("").filename();
  default_filename.replace_filename(btiName);
  std::vector<std::string> filters{"??? (*.bti)", "*.bti"};
  auto results =
      rsl::ReadOneFile("Open File"_j, default_filename.string(), filters);
  if (!results) {
    rsl::ErrorDialog("No file selected");
    return {};
  }

  rsl::trace("Attempting to load from {}", results->path.string());

  librii::j3d::Tex tmp;
  auto reader = TRY(oishii::BinaryReader::FromFilePath(results->path.string(),
                                                       std::endian::big));
  rsl::SafeReader safeReader(reader);
  TRY(tmp.transfer(safeReader));
  auto buffer_addr = tmp.ofsTex;
  auto buffer_size = librii::gx::computeImageSize(
      tmp.mWidth, tmp.mHeight, tmp.mFormat, tmp.mMipmapLevel);

  auto image_data = TRY(reader.tryReadBuffer<u8>(buffer_size, buffer_addr));

  auto block = librii::jpa::TextureBlock(tmp, image_data);
  block.setName(
      std::string(results->path.replace_extension().filename().string()));

  data = block;
  return {};
}

void librii::jpa::exportBTI(const char* btiName,
                            librii::jpa::TextureBlock data) {
  auto default_filename = std::filesystem::path("").filename();
  default_filename.replace_filename(btiName);
  std::vector<std::string> filters{"??? (*.bti)", "*.bti"};
  auto results =
      rsl::SaveOneFile("Save File"_j, default_filename.string(), filters);
  if (!results) {
    rsl::ErrorDialog("No saving - No file selected");
    return;
  }
  auto path = results->string();

  if (!path.ends_with(".bti")) {
    path += ".bti";
  }

  rsl::trace("Attempting to save to {}", path);
  oishii::Writer writer(std::endian::big);
  data.tex.write(writer);
  writer.write(data.tex.ofsTex);
  for (auto& b : data.getData()) {
    writer.write(b);
  }

  writer.saveToDisk(path);
}
