#include "FsDialog.hpp"
#include <plate/Platform.hpp>
#include <vendor/FileDialogues.hpp>

IMPORT_STD;

#include <fstream>

namespace rsl {

bool FileDialogsSupported() { return plate::Platform::supportsFileDialogues(); }

Result<std::filesystem::path> OpenOneFile(std::string_view title,
                                          std::string_view default_path,
                                          std::vector<std::string> filters) {
  const auto paths =
      pfd::open_file(std::string(title), std::string(default_path), filters,
                     /* allow_multiselect */ false)
          .result();
  if (paths.empty()) {
    return std::unexpected("No file was selected");
  }
  if (paths.size() > 1) {
    return std::unexpected("Too many files selected");
  }
  return std::filesystem::path(paths[0]);
}

Result<std::vector<std::filesystem::path>>
OpenManyFiles(std::string_view title, std::string_view default_path,
              std::vector<std::string> filters) {
  const auto paths =
      pfd::open_file(std::string(title), std::string(default_path), filters,
                     /* allow_multiselect */ true)
          .result();
  if (paths.empty()) {
    return std::unexpected("No file was selected");
  }

  std::vector<std::filesystem::path> result;
  for (const auto& path : paths) {
    result.push_back(path);
  }
  return result;
}

Result<std::filesystem::path> OpenFolder(std::string_view title,
                                         std::string_view default_path,
                                         bool must_exist) {
  if (!FileDialogsSupported()) {
    return std::unexpected("File dialogues unsupported on this platform");
  }
  const auto folder =
      pfd::select_folder(std::string(title), std::string(default_path))
          .result();
  if (folder.empty()) {
    return std::unexpected("No folder was selected");
  }
  auto path = std::filesystem::path(folder);
  if (!std::filesystem::exists(path)) {
    if (must_exist)
      return std::unexpected("Folder doesn't exist");
    return path;
  }
  if (!std::filesystem::is_directory(path)) {
    return std::unexpected("Not a folder");
  }
  return path;
}

std::expected<File, std::string> ReadOneFile(std::filesystem::path path) {
  std::ifstream file(path.string(), std::ios::binary | std::ios::ate);
  if (!file) {
    return std::unexpected("Failed to open file \"" + path.string() + "\"");
  }

  std::vector<u8> vec(file.tellg());
  {
    file.seekg(0, std::ios::beg);

    if (!file.read(reinterpret_cast<char*>(vec.data()), vec.size())) {
      return std::unexpected("Failed to read file \"" + path.string() + "\"");
    }
  }
  return File{
      .path = path,
      .data = std::move(vec),
  };
}
Result<File> ReadOneFile(std::string_view title, std::string_view default_path,
                         std::vector<std::string> filters) {
  auto path = TRY(OpenOneFile(title, default_path, filters));
  return ReadOneFile(path);
}

Result<std::vector<File>> ReadManyFile(std::string_view title,
                                       std::string_view default_path,
                                       std::vector<std::string> filters) {
  auto paths = TRY(OpenManyFiles(title, default_path, filters));
  std::vector<File> result;
  for (const auto& path : paths) {
    auto file = TRY(ReadOneFile(path));
    result.push_back(file);
  }
  return result;
}

Result<std::filesystem::path> SaveOneFile(std::string_view title,
                                          std::string_view default_path,
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
    return std::unexpected("No file was selected");
  }
  auto result = std::filesystem::path(path);
  if (filter_index != -1) {
    if (filter_index >= filters.size()) {
      return std::unexpected(
          "Internal error: file dialog component returned invalid filter ID");
    }
    auto& filter = filters[filter_index];
    // We only care about the first filter
    auto f = filter.substr(0, filter.find_first_of(";"));
    auto filter_path = std::filesystem::path(f);
    // We also are ignoring everything before the format
    if (!filter_path.has_extension() && filter_path != "*") {
      return std::unexpected(
          "Internal error: file dialog supplied invalid filter " + filter +
          ". Cannot determine extension.");
    }
    // Note: for now just add it if missing. Can correct it also.
    if (!result.has_extension() && filter_path.has_extension()) {
      result.replace_extension(filter_path.extension());
    }
  }
  return result;
}

void ErrorDialog(const std::string& message) {
  pfd::message("Error"_j, message, pfd::choice::ok, pfd::icon::warning);
}

} // namespace rsl
