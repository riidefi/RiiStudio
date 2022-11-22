#pragma once

#include <core/common.h>
#include <filesystem>
#include <rsl/Expected.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace rsl {

bool FileDialogsSupported();

rsl::expected<std::filesystem::path, std::string>
OpenOneFile(std::string_view title, std::string_view default_path,
            std::vector<std::string> filters);

rsl::expected<std::vector<std::filesystem::path>, std::string>
OpenManyFiles(std::string_view title, std::string_view default_path,
              std::vector<std::string> filters);

struct File {
  std::filesystem::path path;
  std::vector<u8> data;
};

rsl::expected<File, std::string> ReadOneFile(std::filesystem::path path);
rsl::expected<File, std::string> ReadOneFile(std::string_view title,
                                             std::string_view default_path,
                                             std::vector<std::string> filters);

rsl::expected<std::vector<File>, std::string>
ReadManyFile(std::string_view title, std::string_view default_path,
             std::vector<std::string> filters);

rsl::expected<std::filesystem::path, std::string>
SaveOneFile(std::string_view title, std::string_view default_path,
            std::vector<std::string> filters);

} // namespace rsl
