#include "EditorFactory.hpp"

#include <frontend/bdof/AssimpImporter.hpp>
#include <frontend/bdof/BblmEditor.hpp>
#include <frontend/bdof/BdofEditor.hpp>
#include <frontend/bdof/BfgEditor.hpp>
#include <frontend/bdof/BlightEditor.hpp>
#include <frontend/level_editor/LevelEditor.hpp>

#include <frontend/file_host.hpp>
#include <librii/szs/SZS.hpp>

namespace riistudio::frontend {

//! Create an editor from the file data specified. Returns nullptr on failure.
std::unique_ptr<IWindow> MakeEditor(FileData& data) {
  rsl::info("Opening file: {}", data.mPath.c_str());

  std::span<const u8> span(data.mData.get(), data.mData.get() + data.mLen);

  if (data.mPath.ends_with(".szs")) {
    auto pWin = std::make_unique<lvl::LevelEditorWindow>();
    pWin->openFile(span, data.mPath);
    return pWin;
  }
  if (data.mPath.ends_with(".bdof") || data.mPath.ends_with(".pdof")) {
    auto pWin = std::make_unique<BdofEditor>();
    pWin->openFile(span, data.mPath);
    return pWin;
  }
  // .bblm1 .bblm2 should also be matched
  if (data.mPath.contains(".bblm") || data.mPath.ends_with(".pblm")) {
    auto pWin = std::make_unique<BblmEditor>();
    pWin->openFile(span, data.mPath);
    return pWin;
  }
  if (data.mPath.ends_with(".bfg")) {
    auto pWin = std::make_unique<BfgEditor>(span, data.mPath);
    return pWin;
  }
  if (data.mPath.ends_with(".blight") || data.mPath.ends_with(".plight")) {
    return std::make_unique<BlightEditor>(span, data.mPath);
  }
  if (AssimpImporter::supports(data.mPath)) {
    return std::make_unique<AssimpImporter>(span, data.mPath);
  }

  return nullptr;
}

std::optional<std::vector<uint8_t>> LoadLuigiCircuitSample() {
  auto szs = ReadFileData("./samp/luigi_circuit_brres.szs");
  if (!szs)
    return std::nullopt;

  rsl::byte_view szs_view{szs->mData.get(), szs->mLen};
  const auto expanded_size = librii::szs::getExpandedSize(szs_view);
  if (!expanded_size)
    return std::nullopt;

  std::vector<uint8_t> brres(*expanded_size);
  auto err = librii::szs::decode(brres, szs_view);
  if (!err)
    return std::nullopt;

  return brres;
}

} // namespace riistudio::frontend
