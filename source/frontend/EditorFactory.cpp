#include "EditorFactory.hpp"

#include <frontend/editors/AssimpImporter.hpp>
#include <frontend/editors/BblmEditor.hpp>
#include <frontend/editors/BdofEditor.hpp>
#include <frontend/editors/BfgEditor.hpp>
#include <frontend/editors/BlightEditor.hpp>
#include <frontend/editors/BlmapEditor.hpp>
#include <frontend/editors/BtkEditor.hpp>
#include <frontend/editors/RarcEditor.hpp>
#include <frontend/legacy_editor/EditorWindow.hpp>
#include <frontend/level_editor/LevelEditor.hpp>

#include <frontend/file_host.hpp>
#include <librii/szs/SZS.hpp>
#include <librii/arc/RARC.hpp>
#include <librii/u8/U8.hpp>

namespace riistudio::frontend {

//! Create an editor from the file data specified. Returns nullptr on failure.
std::unique_ptr<IWindow> MakeEditor(FileData& data) {
  rsl::info("Opening file: {}", data.mPath.c_str());

  std::span<u8> raw_span(data.mData.get(), data.mData.get() + data.mLen);

  std::vector<u8> expanded_span;
  if (librii::szs::isDataYaz0Compressed(raw_span)) {
    expanded_span.resize(*librii::szs::getExpandedSize(raw_span));
    if (!librii::szs::decode(expanded_span, raw_span))
      return nullptr;
  } else {
    expanded_span = {raw_span.data(), raw_span.data() + raw_span.size()};
  }

  auto path_lower = data.mPath;
  std::transform(path_lower.begin(), path_lower.end(), path_lower.begin(),
                 ::tolower);

  if (path_lower.ends_with(".szs")) {
    auto pWin = std::make_unique<lvl::LevelEditorWindow>();
    pWin->openFile(expanded_span, data.mPath);
    return pWin;
  }
  if (path_lower.ends_with(".bdof") || path_lower.ends_with(".pdof")) {
    auto pWin = std::make_unique<BdofEditor>();
    pWin->openFile(expanded_span, data.mPath);
    return pWin;
  }
  // .bblm1 .bblm2 should also be matched
  if (path_lower.contains(".bblm") || path_lower.ends_with(".pblm")) {
    auto pWin = std::make_unique<BblmEditor>();
    pWin->openFile(expanded_span, data.mPath);
    return pWin;
  }
  if (path_lower.ends_with(".bfg")) {
    auto pWin = std::make_unique<BfgEditor>(expanded_span, data.mPath);
    return pWin;
  }
  if (path_lower.ends_with(".blight") || path_lower.ends_with(".plight")) {
    return std::make_unique<BlightEditor>(expanded_span, data.mPath);
  }
  if (path_lower.ends_with(".blmap") || path_lower.ends_with(".plmap")) {
    return std::make_unique<BlmapEditor>(expanded_span, data.mPath);
  }
  if (path_lower.ends_with(".btk")) {
    auto pWin = std::make_unique<BtkEditor>();
    pWin->openFile(expanded_span, data.mPath);
    return pWin;
  }
  if (path_lower.ends_with(".arc") || path_lower.ends_with(".carc") ||
      path_lower.ends_with(".u8")) {
    if (librii::RARC::IsDataResourceArchive(expanded_span)) {
      auto pWin = std::make_unique<RarcEditor>();
      pWin->openFile(expanded_span, data.mPath);
      return pWin;
    } else if (librii::U8::IsDataU8Archive(expanded_span)) {
      /*auto pWin = std::make_unique<U8Editor>();
      pWin->openFile(expanded_span, data.mPath);
      return pWin;*/
    }
  }
  if (path_lower.ends_with(".brres")) {
    auto pWin = std::make_unique<BRRESEditor>(span, data.mPath);
    return pWin;
  }
  if (path_lower.ends_with(".bmd") || path_lower.ends_with(".bdl")) {
    auto pWin = std::make_unique<BMDEditor>(span, data.mPath);
    return pWin;
  }
  if (AssimpImporter::supports(path_lower)) {
    return std::make_unique<AssimpImporter>(expanded_span, data.mPath);
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
