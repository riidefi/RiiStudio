#include "FsDialog.hpp"
#include <fstream>
#include <plate/Platform.hpp>
#include <ranges>
#include <vendor/FileDialogues.hpp>

namespace rsl {

bool FileDialogsSupported() { return plate::Platform::supportsFileDialogues(); }

rsl::expected<std::filesystem::path, std::string>
OpenOneFile(std::string_view title, std::string_view default_path,
            std::vector<std::string> filters) {
  const auto paths =
      pfd::open_file(std::string(title), std::string(default_path), filters,
                     /* allow_multiselect */ false)
          .result();
  if (paths.empty()) {
    return std::string("No file was selected");
  }
  if (paths.size() > 1) {
    return std::string("Too many files selected");
  }
  return std::filesystem::path(paths[0]);
}

rsl::expected<std::vector<std::filesystem::path>, std::string>
OpenManyFiles(std::string_view title, std::string_view default_path,
              std::vector<std::string> filters) {
  const auto paths =
      pfd::open_file(std::string(title), std::string(default_path), filters,
                     /* allow_multiselect */ true)
          .result();
  if (paths.empty()) {
    return std::string("No file was selected");
  }

  std::vector<std::filesystem::path> result;
  for (const auto& path : paths) {
    result.push_back(path);
  }
  return result;
}
rsl::expected<File, std::string> ReadOneFile(std::filesystem::path path) {
  std::ifstream file(path.string(), std::ios::binary | std::ios::ate);
  if (!file) {
    return std::string("Failed to open file \"" + path.string() + "\"");
  }

  std::vector<u8> vec(file.tellg());
  {
    file.seekg(0, std::ios::beg);

    if (!file.read(reinterpret_cast<char*>(vec.data()), vec.size())) {
      return std::string("Failed to read file \"" + path.string() + "\"");
    }
  }
  return File{
      .path = path,
      .data = std::move(vec),
  };
}
rsl::expected<File, std::string> ReadOneFile(std::string_view title,
                                             std::string_view default_path,
                                             std::vector<std::string> filters) {
  auto path = OpenOneFile(title, default_path, filters);
  if (!path) {
    return path.error();
  }
  return ReadOneFile(*path);
}

rsl::expected<std::vector<File>, std::string>
ReadManyFile(std::string_view title, std::string_view default_path,
             std::vector<std::string> filters) {
  auto paths = OpenManyFiles(title, default_path, filters);
  if (!paths) {
    return paths.error();
  }
  std::vector<File> result;
  for (const auto& path : *paths) {
    auto file = ReadOneFile(path);
    if (!file) {
      return file.error();
    }
    result.push_back(*file);
  }
  return result;
}

rsl::expected<std::filesystem::path, std::string>
SaveOneFile(std::string_view title, std::string_view default_path,
            std::vector<std::string> filters) {
  int filter_index = -1;
  // Disable for now, this breaks the dialog silently..
#ifdef __APPLE__
  default_path = "";
#endif
  const auto path =
      pfd::save_file(std::string(title), std::string(default_path), filters,
                     /* confirm_overwrite */ true, &filter_index)
          .result();
  if (path.empty()) {
    return std::string("No file was selected");
  }
  auto result = std::filesystem::path(path);
  if (filter_index != -1) {
    if (filter_index >= filters.size()) {
      return std::string(
          "Internal error: file dialog component returned invalid filter ID");
    }
    auto& filter = filters[filter_index];
    // We only care about the first filter
    auto f = filter.substr(0, filter.find_first_of(";"));
    auto filter_path = std::filesystem::path(f);
    // We also are ignoring everything before the format
    if (!filter_path.has_extension()) {
      return std::string(
          "Internal error: file dialog supplied invalid filter " + filter +
          ". Cannot determine extension.");
    }
    // Note: for now just add it if missing. Can correct it also.
    if (!result.has_extension()) {
      result.replace_extension(filter_path.extension());
    }
  }
  return result;
}

} // namespace rsl
