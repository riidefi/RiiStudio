#include "Cli.hpp"
#include <core/util/oishii.hpp>
#include <core/util/timestamp.hpp>
#include <fmt/color.h>
#include <iostream>
#include <librii/assimp/LRAssimp.hpp>
#include <librii/assimp2rhst/Assimp.hpp>
#include <librii/assimp2rhst/SupportedFiles.hpp>
#include <librii/crate/g3d_crate.hpp>
#include <librii/crate/j3d_crate.hpp>
#include <librii/g3d/io/TextureIO.hpp>
#include <librii/j3d/PreciseBMDDump.hpp>
#include <librii/kcol/Model.hpp>
#include <librii/kmp/io/KMP.hpp>
#include <librii/rarc/RARC.hpp>
#include <librii/rhst/MeshUtils.hpp>
#include <librii/rhst/RHSTOptimizer.hpp>
#include <librii/szs/SZS.hpp>
#include <librii/u8/U8.hpp>
#include <mutex>
#include <plugins/g3d/G3dIo.hpp>
#include <plugins/g3d/collection.hpp>
#include <plugins/j3d/J3dIo.hpp>
#include <plugins/j3d/Preset.hpp>
#include <plugins/rhst/RHSTImporter.hpp>
#include <rsl/Filesystem.hpp>
#include <rsl/Stb.hpp>
#include <rsl/StringManip.hpp>
#include <rsl/Timer.hpp>
#include <rsl/WriteFile.hpp>
#include <sstream>

namespace riistudio {
const char* translateString(std::string_view str) { return str.data(); }
} // namespace riistudio

namespace llvm {
int EnableABIBreakingChecks;
int DisableABIBreakingChecks;
} // namespace llvm

static void setFlag(u32& f, u32 m, bool sel) {
  if (sel) {
    f |= m;
  } else {
    f &= ~m;
  }
}

std::mutex s_progressLock;

static void progress_put(std::string status, float percent, int barWidth = 70) {
  std::stringstream ss;
  ss << status;
  for (size_t i = status.size(); i < 32; ++i) {
    ss << " ";
  }
  ss << "[";
  int pos = static_cast<int>(static_cast<float>(barWidth) * percent);
  for (int i = 0; i < barWidth; ++i) {
    if (i < pos)
      ss << "=";
    else if (i == pos)
      ss << ">";
    else
      ss << " ";
  }
  ss << "] " << static_cast<int>(percent * 100.0f) << " %"
     << "\r";
  std::unique_lock g(s_progressLock);
  std::cout << ss.str();
  std::cout.flush();
}
static void progress_end() {
  std::unique_lock g(s_progressLock);
  std::cout << std::endl;
}

static auto GetMips(const CliOptions& opt)
    -> std::optional<riistudio::rhst::MipGen> {
  if (!opt.mipmaps) {
    return std::nullopt;
  }
  return riistudio::rhst::MipGen{
      .min_dim = opt.min_mip,
      .max_mip = opt.max_mips,
  };
}

#define FS_TRY(expr)                                                           \
  TRY(expr.transform_error([](const std::error_code& ec) -> std::string {      \
    return std::format("Filesystem Error: {} ({}:{})", ec.message(), __FILE__, \
                       __LINE__);                                              \
  }))

class ImportBRRES {
public:
  ImportBRRES(const CliOptions& opt) : m_opt(opt) {}

  Result<void> execute() {
    if (m_opt.verbose) {
      rsl::logging::init();
    }
    auto pok = parseArgs();
    if (!pok) {
      return std::unexpected("Failed to parse args: " + pok.error());
    }
    if (!librii::assimp2rhst::IsExtensionSupported(m_from.string())) {
      return std::unexpected("File format is unsupported");
    }
    auto file = ReadFile(m_opt.from.view());
    if (!file.has_value()) {
      return std::unexpected("Failed to read file");
    }
    const auto on_log = [](kpi::IOMessageClass c, std::string_view d,
                           std::string_view b) {
      rsl::info("Assimp message: {} {} {}", magic_enum::enum_name(c), d, b);
    };
    auto settings = getSettings();
    if (m_opt.ai_json) {
      Assimp::Importer importer;
      auto* pScene = ReadScene(on_log, std::span(*file), m_from.string(),
                               settings, importer);
      if (!pScene) {
        return std::unexpected("Failed to read ASSIMP SCENE");
      }
      auto scn = librii::lra::ReadScene(*pScene);
      auto s = librii::lra::PrintJSON(scn);
      std::ofstream out(m_to.string());
      out << s;
      fmt::print(stdout, "Dumped ASSIMP json\n");
      return {};
    }
    auto tree = librii::assimp2rhst::DoImport(m_from.string(), on_log,
                                              std::span(*file), settings);
    if (!tree) {
      return std::unexpected("Failed to parse: " + tree.error());
    }
    auto progress = [&](std::string_view s, float f) {
      progress_put(std::string(s), f);
    };
    auto info = [&](std::string c, std::string v) {
      on_log(kpi::IOMessageClass::Information, c, v);
    };
    auto m_result = std::make_unique<riistudio::g3d::Collection>();
    bool ok = riistudio::rhst::CompileRHST(*tree, *m_result, m_from.string(),
                                           info, progress, GetMips(m_opt),
                                           !m_opt.no_tristrip, m_opt.verbose);
    if (!ok) {
      return std::unexpected("Failed to parse RHST");
    }
    std::unordered_map<std::string, librii::crate::CrateAnimation> presets;
    if (FS_TRY(rsl::filesystem::exists(m_presets)) &&
        FS_TRY(rsl::filesystem::is_directory(m_presets))) {
      for (auto it : std::filesystem::directory_iterator(m_presets)) {
        if (it.path().extension() == ".rspreset") {
          auto file = ReadFile(it.path().string());
          if (!file) {
            fmt::print(stderr, "Failed to read rspreset: {}\n",
                       it.path().string());
            continue;
          }
          auto preset = librii::crate::ReadRSPreset(*file);
          if (!preset) {
            fmt::print(stderr, "Failed to parse rspreset: {}\n",
                       it.path().string());
            continue;
          }
          presets[it.path().stem().string()] = *preset;
        }
      }
      auto& mdl = m_result->getModels()[0];
      for (size_t i = 0; i < mdl.getMaterials().size(); ++i) {
        auto& target_mat = mdl.getMaterials()[i];
        if (!presets.contains(target_mat.name))
          continue;
        auto& source_mat = presets[target_mat.name];
        auto ok =
            riistudio::g3d::ApplyCratePresetToMaterial(target_mat, source_mat);
        if (!ok) {
          fmt::print(stderr,
                     "Attempted to merge {} into {}. Failed with error: {}\n",
                     source_mat.mat.name, target_mat.name, ok.error());
        }
      }
    }
    oishii::Writer result(std::endian::big);
    TRY(riistudio::g3d::WriteBRRES(*m_result, result));
    result.saveToDisk(m_to.string());
    return {};
  }

private:
  Result<void> parseArgs() {
    m_from = m_opt.from.view();
    m_to = m_opt.to.view();
    m_presets = m_opt.preset_path.view();
    if (m_to.empty()) {
      std::filesystem::path p = m_from;
      p.replace_extension(".brres");
      m_to = p;
    }
    if (!FS_TRY(rsl::filesystem::exists(m_from))) {
      fmt::print(stderr, "Error: File {} does not exist.\n", m_from.string());
      return std::unexpected("FileNotExist");
    }
    if (!FS_TRY(rsl::filesystem::exists(m_to))) {
      fmt::print(stderr,
                 "Warning: File {} will be overwritten by this operation.\n",
                 m_to.string());
    }
    if (!m_presets.empty()) {
      if (!FS_TRY(rsl::filesystem::exists(m_presets))) {
        fmt::print(stderr,
                   "Warning: Preset path {} does not exist not valid.\n",
                   m_presets.string());
        m_presets.clear();
      } else if (!FS_TRY(rsl::filesystem::is_directory(m_presets))) {
        fmt::print(stderr,
                   "Warning: Preset path {} does not refer to a folder.\n",
                   m_presets.string());
        m_presets.clear();
      } else {
        fmt::print(stdout, "Info: Reading presets from {}\n",
                   m_presets.string());
      }
    }
    return {};
  }

  librii::assimp2rhst::Settings getSettings() {
    librii::assimp2rhst::Settings settings;
    auto& f = settings.mAiFlags;
    setFlag(f, aiProcess_RemoveRedundantMaterials, m_opt.merge_mats);
    setFlag(f, aiProcess_TransformUVCoords, m_opt.bake_uvs);
    setFlag(f, aiProcess_FindDegenerates, m_opt.cull_degenerates);
    setFlag(f, aiProcess_FindInvalidData, m_opt.cull_invalid);
    setFlag(f, aiProcess_FixInfacingNormals, m_opt.recompute_normals);
    setFlag(f, aiProcess_OptimizeMeshes, true);
    setFlag(f, aiProcess_OptimizeGraph, true);
    settings.mMagnification = m_opt.scale;
    glm::vec3 tint;
    tint.r = static_cast<float>((m_opt.hexcode >> 16) & 0xff) / 255.0f;
    tint.g = static_cast<float>((m_opt.hexcode >> 8) & 0xff) / 255.0f;
    tint.b = static_cast<float>((m_opt.hexcode >> 0) & 0xff) / 255.0f;
    settings.mModelTint = tint;
    settings.mGenerateMipMaps = m_opt.mipmaps;
    settings.mMinMipDimension = m_opt.min_mip;
    settings.mMaxMipCount = m_opt.max_mips;
    settings.mAutoTransparent = m_opt.auto_transparency;
    settings.mIgnoreRootTransform = m_opt.brawlbox_scale;
    return settings;
  }

  CliOptions m_opt;
  std::filesystem::path m_from;
  std::filesystem::path m_to;
  std::filesystem::path m_presets;
};

class ImportBMD {
public:
  ImportBMD(const CliOptions& opt) : m_opt(opt) {}

  Result<void> execute() {
    if (m_opt.verbose) {
      rsl::logging::init();
    }
    auto pok = parseArgs();
    if (!pok) {
      return std::unexpected("Failed to parse args: " + pok.error());
    }
    if (!librii::assimp2rhst::IsExtensionSupported(m_from.string())) {
      return std::unexpected("File format is unsupported");
    }
    auto file = ReadFile(m_opt.from.view());
    if (!file.has_value()) {
      return std::unexpected("Failed to read file");
    }
    const auto on_log = [](kpi::IOMessageClass c, std::string_view d,
                           std::string_view b) {
      rsl::info("Assimp message: {} {} {}", magic_enum::enum_name(c), d, b);
    };
    auto settings = getSettings();
    if (m_opt.ai_json) {
      Assimp::Importer importer;
      auto* pScene = ReadScene(on_log, std::span(*file), m_from.string(),
                               settings, importer);
      if (!pScene) {
        return std::unexpected("Failed to read ASSIMP SCENE");
      }
      auto scn = librii::lra::ReadScene(*pScene);
      auto s = librii::lra::PrintJSON(scn);
      std::ofstream out(m_to.string());
      out << s;
      fmt::print(stdout, "Dumped ASSIMP json\n");
      return {};
    }
    auto tree = librii::assimp2rhst::DoImport(m_from.string(), on_log,
                                              std::span(*file), settings);
    if (!tree) {
      return std::unexpected("Failed to parse: " + tree.error());
    }
    auto progress = [&](std::string_view s, float f) {
      progress_put(std::string(s), f);
    };
    auto info = [&](std::string c, std::string v) {
      on_log(kpi::IOMessageClass::Information, c, v);
    };
    auto m_result = std::make_unique<riistudio::j3d::Collection>();
    bool ok = riistudio::rhst::CompileRHST(*tree, *m_result, m_from.string(),
                                           info, progress, GetMips(m_opt),
                                           !m_opt.no_tristrip, m_opt.verbose);
    if (!ok) {
      return std::unexpected("Failed to parse RHST");
    }
    std::unordered_map<std::string, librii::crate::CrateAnimationJ3D> presets;
    if (FS_TRY(rsl::filesystem::exists(m_presets)) &&
        FS_TRY(rsl::filesystem::is_directory(m_presets))) {
      for (auto it : std::filesystem::directory_iterator(m_presets)) {
        if (it.path().extension() == ".bmd_rspreset") {
          auto file = ReadFile(it.path().string());
          if (!file) {
            fmt::print(stderr, "Failed to read bmd_rspreset: {}\n",
                       it.path().string());
            continue;
          }
          auto preset = librii::crate::ReadRSPresetJ3D(*file);
          if (!preset) {
            fmt::print(stderr, "Failed to parse bmd_rspreset: {}\n",
                       it.path().string());
            continue;
          }
          presets[it.path().stem().string()] = *preset;
        }
      }
      auto& mdl = m_result->getModels()[0];
      for (size_t i = 0; i < mdl.getMaterials().size(); ++i) {
        auto& target_mat = mdl.getMaterials()[i];
        if (!presets.contains(target_mat.name))
          continue;
        auto& source_mat = presets[target_mat.name];
        auto ok =
            ApplyCratePresetToMaterialJ3D(*m_result, target_mat, source_mat);
        if (!ok) {
          fmt::print(stderr,
                     "Attempted to merge {} into {}. Failed with error: {}\n",
                     source_mat.mat.name, target_mat.name, ok.error());
          continue;
        }
        auto s = std::format("Applied preset {}", source_mat.mat.name);
        fmt::print(stdout, "{}\n", fmt::styled(s, fmt::fg(fmt::color::green)));
      }
    }
    oishii::Writer result(std::endian::big);
    TRY(riistudio::j3d::WriteBMD(*m_result, result, false));
    result.saveToDisk(m_to.string());
    return {};
  }

private:
  Result<void> parseArgs() {
    m_from = m_opt.from.view();
    m_to = m_opt.to.view();
    m_presets = m_opt.preset_path.view();
    if (m_to.empty()) {
      std::filesystem::path p = m_from;
      p.replace_extension(".bmd");
      m_to = p;
    }
    if (!FS_TRY(rsl::filesystem::exists(m_from))) {
      fmt::print(stderr, "Error: File {} does not exist.\n", m_from.string());
      return std::unexpected("FileNotExist");
    }
    if (!FS_TRY(rsl::filesystem::exists(m_to))) {
      fmt::print(stderr,
                 "Warning: File {} will be overwritten by this operation.\n",
                 m_to.string());
    }
    if (!m_presets.empty()) {
      if (!FS_TRY(rsl::filesystem::exists(m_presets))) {
        fmt::print(stderr,
                   "Warning: Preset path {} does not exist not valid.\n",
                   m_presets.string());
        m_presets.clear();
      } else if (!FS_TRY(rsl::filesystem::is_directory(m_presets))) {
        fmt::print(stderr,
                   "Warning: Preset path {} does not refer to a folder.\n",
                   m_presets.string());
        m_presets.clear();
      } else {
        fmt::print(stdout, "Info: Reading presets from {}\n",
                   m_presets.string());
      }
    }
    return {};
  }

  librii::assimp2rhst::Settings getSettings() {
    librii::assimp2rhst::Settings settings;
    auto& f = settings.mAiFlags;
    setFlag(f, aiProcess_RemoveRedundantMaterials, m_opt.merge_mats);
    setFlag(f, aiProcess_TransformUVCoords, m_opt.bake_uvs);
    setFlag(f, aiProcess_FindDegenerates, m_opt.cull_degenerates);
    setFlag(f, aiProcess_FindInvalidData, m_opt.cull_invalid);
    setFlag(f, aiProcess_FixInfacingNormals, m_opt.recompute_normals);
    setFlag(f, aiProcess_OptimizeMeshes, true);
    setFlag(f, aiProcess_OptimizeGraph, true);
    settings.mMagnification = m_opt.scale;
    glm::vec3 tint;
    tint.r = static_cast<float>((m_opt.hexcode >> 16) & 0xff) / 255.0f;
    tint.g = static_cast<float>((m_opt.hexcode >> 8) & 0xff) / 255.0f;
    tint.b = static_cast<float>((m_opt.hexcode >> 0) & 0xff) / 255.0f;
    settings.mModelTint = tint;
    settings.mGenerateMipMaps = m_opt.mipmaps;
    settings.mMinMipDimension = m_opt.min_mip;
    settings.mMaxMipCount = m_opt.max_mips;
    settings.mAutoTransparent = m_opt.auto_transparency;
    settings.mIgnoreRootTransform = m_opt.brawlbox_scale;
    return settings;
  }

  CliOptions m_opt;
  std::filesystem::path m_from;
  std::filesystem::path m_to;
  std::filesystem::path m_presets;
};

class DecompressSZS {
public:
  DecompressSZS(const CliOptions& opt) : m_opt(opt) {}

  Result<void> execute() {
    if (m_opt.verbose) {
      rsl::logging::init();
    }
    auto pok = parseArgs();
    if (!pok) {
      return std::unexpected("Error: failed to parse args: " + pok.error());
    }
    fmt::print(stderr, "Decompressing SZS: {} => {}\n", m_from.string(),
               m_to.string());
    auto file = ReadFile(m_opt.from.view());
    if (!file.has_value()) {
      return std::unexpected("Error: Failed to read file");
    }
    // Max 4GB
    u32 size = TRY(librii::szs::getExpandedSize(*file));
    std::vector<u8> buf(size);
    TRY(librii::szs::decode(buf, *file));
    if (FS_TRY(rsl::filesystem::is_directory(m_to))) {
      return std::unexpected("Failed to extract: |to| is a folder, not a file");
    }
    TRY(rsl::WriteFile(buf, m_to.string()));
    return {};
  }

private:
  Result<void> parseArgs() {
    m_from = std::string{m_opt.from.view()};
    m_to = std::string{m_opt.to.view()};

    if (m_to.empty()) {
      std::filesystem::path p = m_from;
      p.replace_extension(".arc");
      m_to = p;
    }
    if (!FS_TRY(rsl::filesystem::exists(m_from))) {
      fmt::print(stderr, "Error: File {} does not exist.\n", m_from.string());
      return std::unexpected("FileNotExist");
    }
    if (FS_TRY(rsl::filesystem::exists(m_to))) {
      fmt::print(stderr,
                 "Warning: Folder {} will be overwritten by this operation.\n",
                 m_to.string());
    }
    return {};
  }

  CliOptions m_opt;
  std::filesystem::path m_from;
  std::filesystem::path m_to;
};
class CompressSZS {
public:
  CompressSZS(const CliOptions& opt) : m_opt(opt) {}

  Result<void> execute() {
    if (m_opt.verbose) {
      rsl::logging::init();
    }
    auto pok = parseArgs();
    if (!pok) {
      return std::unexpected(
          std::format("Error: failed to parse args: {}", pok.error()));
    }
    auto from = FS_TRY(rsl::filesystem::absolute(m_from));
    auto file = ReadFile(from.string());
    if (!file.has_value()) {
      return std::unexpected(
          std::format("Error: Failed to read file {}\n", from.string()));
    }
    auto strat = TRY(rsl::enum_cast<librii::szs::Algo>(m_opt.szs_algo));
    auto sname = magic_enum::enum_name(strat);
    rsl::Timer timer;
    timer.reset();
    fmt::print(stderr, "Compressing SZS: {} => {} ({} strategy)\n",
               m_from.string(), m_to.string(),
               fmt::styled(sname, fmt::fg(fmt::color::gold)));
    auto buf = TRY(librii::szs::encodeAlgo(*file, strat));
    float elapsed = static_cast<float>(timer.elapsed()) * 0.001f;
    float rate =
        static_cast<float>(buf.size()) / static_cast<float>(file->size());
    buf.resize(roundUp(buf.size(), 32));
    TRY(rsl::WriteFile(buf, m_to.string()));

    fmt::print("Elapsed time: {:.2f} seconds. Compression rate: {:.2f}% "
               "(lower is better)",
               fmt::styled(elapsed, fmt::fg(fmt::color::light_green)),
               fmt::styled(rate * 100.0f, fmt::fg(fmt::color::light_green)));

    return {};
  }

private:
  Result<void> parseArgs() {
    m_from = m_opt.from.view();
    m_to = m_opt.to.view();

    if (m_to.empty()) {
      std::filesystem::path p = m_from;
      p.replace_extension(".szs");
      m_to = p;
    }
    if (!FS_TRY(rsl::filesystem::exists(m_from))) {
      fmt::print(stderr, "Error: File {} does not exist.\n", m_from.string());
      return std::unexpected("FileNotExist");
    }
    if (FS_TRY(rsl::filesystem::exists(m_to))) {
      fmt::print(stderr,
                 "Warning: File {} will be overwritten by this operation.\n",
                 m_to.string());
    }
    return {};
  }

  CliOptions m_opt;
  std::filesystem::path m_from;
  std::filesystem::path m_to;
};

[[nodiscard]] Result<void> WriteIt(riistudio::g3d::Collection& c,
                                   oishii::Writer& w) {
  return riistudio::g3d::WriteBRRES(c, w);
}
std::string ExOf(riistudio::g3d::Collection* c) { return ".brres"; }
[[nodiscard]] Result<void> WriteIt(riistudio::j3d::Collection& c,
                                   oishii::Writer& w) {
  return riistudio::j3d::WriteBMD(c, w);
}
std::string ExOf(riistudio::j3d::Collection* c) { return ".bmd"; }

template <typename T> class CompileRHST {
public:
  CompileRHST(const CliOptions& opt) : m_opt(opt) {}

  Result<void> execute() {
    if (m_opt.verbose) {
      rsl::logging::init();
    }
    auto pok = parseArgs();
    if (!pok) {
      return std::unexpected("Failed to parse args: " + pok.error());
    }
    if (m_from.extension() != ".rhst") {
      return std::unexpected("File format is unsupported");
    }
    auto file = ReadFile(m_opt.from.view());
    if (!file.has_value()) {
      return std::unexpected("Failed to read file");
    }
    auto tree = librii::rhst::ReadSceneTree(*file);
    if (!tree) {
      return std::unexpected("Failed to parse RHST");
    }
    auto progress = [&](std::string_view s, float f) {
      progress_put(std::string(s), f);
    };
    const auto on_log = [](kpi::IOMessageClass c, std::string_view d,
                           std::string_view b) {
      rsl::info("message: {} {} {}", magic_enum::enum_name(c), d, b);
    };
    auto info = [&](std::string c, std::string v) {
      on_log(kpi::IOMessageClass::Information, c, v);
    };
    auto m_result = std::make_unique<T>();
    bool ok = riistudio::rhst::CompileRHST(*tree, *m_result, m_from.string(),
                                           info, progress, GetMips(m_opt),
                                           !m_opt.no_tristrip, m_opt.verbose);
    if (!ok) {
      return std::unexpected("Failed to compile RHST");
    }
    oishii::Writer result(std::endian::big);
    TRY(WriteIt(*m_result, result));
    result.saveToDisk(m_to.string());
    return {};
  }

private:
  Result<void> parseArgs() {
    m_from = m_opt.from.view();
    m_to = m_opt.to.view();
    if (m_to.empty()) {
      std::filesystem::path p = m_from;
      p.replace_extension(ExOf((T*)nullptr));
      m_to = p;
    }
    if (!FS_TRY(rsl::filesystem::exists(m_from))) {
      fmt::print(stderr, "Error: File {} does not exist.\n", m_from.string());
      return std::unexpected("FileNotExist");
    }
    if (FS_TRY(rsl::filesystem::exists(m_to))) {
      fmt::print(stderr,
                 "Warning: File {} will be overwritten by this operation.\n",
                 m_to.string());
    }
    return {};
  }

  CliOptions m_opt;
  std::filesystem::path m_from;
  std::filesystem::path m_to;
};

class ExtractArchive {
public:
  ExtractArchive(const CliOptions& opt) : m_opt(opt) {}

  Result<void> execute() {
    if (m_opt.verbose) {
      rsl::logging::init();
    }
    auto pok = parseArgs();
    if (!pok) {
      return std::unexpected("Error: failed to parse args: " + pok.error());
    }
    auto file = ReadFile(m_opt.from.view());
    if (!file.has_value()) {
      return std::unexpected("Error: Failed to read file");
    }

    // Max 4GB
    std::vector<u8> buf;
    if (librii::szs::isDataYaz0Compressed(*file)) {
      u32 size = TRY(librii::szs::getExpandedSize(*file));
      buf.resize(size);
      TRY(librii::szs::decode(buf, *file));
    } else {
      buf = std::move(*file);
    }

    fmt::print(stderr, "Extracting ARC.SZS,{} => {}\n", m_from.string(),
               m_to.string());

    if (librii::RARC::IsDataResourceArchive(buf)) {
      auto arc = TRY(librii::RARC::LoadResourceArchive(buf));
      TRY(librii::RARC::ExtractResourceArchive(arc, m_to));
    } else if (librii::U8::IsDataU8Archive(buf)) {
      auto arc = TRY(librii::U8::LoadU8Archive(buf));
      TRY(librii::U8::Extract(arc, m_to));
    } else {
      return std::unexpected("Error: Archive is neither RARC nor U8");
    }

    return {};
  }

private:
  Result<void> parseArgs() {
    m_from = m_opt.from.view();
    m_to = m_opt.to.view();

    if (m_to.empty()) {
      m_to = FS_TRY(rsl::filesystem::current_path());
    }
    if (!FS_TRY(rsl::filesystem::exists(m_from))) {
      fmt::print(stderr, "Error: File {} does not exist.\n", m_from.string());
      return std::unexpected("FileNotExist");
    }
    if (FS_TRY(rsl::filesystem::exists(m_to))) {
      fmt::print(stderr,
                 "Warning: Folder {} will be overwritten by this operation.\n",
                 m_to.string());
    }
    return {};
  }

  CliOptions m_opt;
  std::filesystem::path m_from;
  std::filesystem::path m_to;
};

class CreateArchive {
public:
  CreateArchive(const CliOptions& opt) : m_opt(opt) {}

  Result<void> execute() {
    if (m_opt.verbose) {
      rsl::logging::init();
    }
    auto pok = parseArgs();
    if (!pok) {
      return std::unexpected("Error: failed to parse args: " + pok.error());
    }
    if (!std::filesystem::is_directory(m_from)) {
      return std::unexpected("Expected a folder, not a file as input");
    }
    fmt::print(stderr, "Creating ARC.SZS,{} => {}\n", m_from.string(),
               std::filesystem::absolute(m_to).string());

    std::vector<u8> buf;
    if (m_opt.rarc) {
      auto arc = TRY(librii::RARC::CreateResourceArchive(m_from));
      buf = TRY(librii::RARC::SaveResourceArchive(arc));
    } else {
      auto arc = TRY(librii::U8::Create(m_from));
      buf = librii::U8::SaveU8Archive(arc);
    }

    if (m_opt.no_compression) {
      buf.resize(roundUp(buf.size(), 32));
      TRY(rsl::WriteFile(buf, m_to.string()));
      return {};
    }

    std::vector<u8> szs(librii::szs::getWorstEncodingSize(buf));
    fmt::print(stderr,
               "Compressing SZS: {} => {} (Boyer-Moore-Horspool strategy)\n",
               m_from.string(), m_to.string());
    int sz = librii::szs::encodeBoyerMooreHorspool(buf.data(), szs.data(),
                                                   buf.size());
    if (sz <= 0 || sz > szs.size()) {
      return std::unexpected("SZS encoding failed");
    }
    szs.resize(roundUp(sz, 32));

    TRY(rsl::WriteFile(szs, m_to.string()));

    return {};
  }

private:
  Result<void> parseArgs() {
    m_from = m_opt.from.view();
    m_to = m_opt.to.view();

    if (m_to.empty()) {
      std::filesystem::path p = m_from;
      p.replace_extension(".d");
      m_to = p;
    }
    if (!FS_TRY(rsl::filesystem::exists(m_from))) {
      fmt::print(stderr, "Error: Folder {} does not exist.\n", m_from.string());
      return std::unexpected("FolderNotExist");
    }
    if (FS_TRY(rsl::filesystem::exists(m_to))) {
      fmt::print(stderr,
                 "Warning: File {} will be overwritten by this operation.\n",
                 m_to.string());
    }
    return {};
  }

  CliOptions m_opt;
  std::filesystem::path m_from;
  std::filesystem::path m_to;
};

static Result<void> kmp2json(const CliOptions& m_opt) {
  if (m_opt.verbose) {
    rsl::logging::init();
  }
  std::filesystem::path m_from = m_opt.from.view();
  std::filesystem::path m_to = m_opt.to.view();

  if (m_to.empty()) {
    std::filesystem::path p = m_from;
    p.replace_extension(".json");
    m_to = p;
  }
  if (!FS_TRY(rsl::filesystem::exists(m_from))) {
    fmt::print(stderr, "Error: File {} does not exist.\n", m_from.string());
    return std::unexpected("FolderNotExist");
  }
  if (FS_TRY(rsl::filesystem::exists(m_to))) {
    fmt::print(stderr,
               "Warning: File {} will be overwritten by this operation.\n",
               m_to.string());
  }
  auto file = ReadFile(m_opt.from.view());
  if (!file.has_value()) {
    return std::unexpected("Error: Failed to read file");
  }
  auto kmp = TRY(librii::kmp::readKMP(*file));

  auto result = DumpJSON(kmp);
  fmt::print("{}\n", result);
  std::ofstream stream(m_to);
  stream << result;

  return {};
}
static Result<void> json2kmp(const CliOptions& m_opt) {
  if (m_opt.verbose) {
    rsl::logging::init();
  }
  std::filesystem::path m_from = m_opt.from.view();
  std::filesystem::path m_to = m_opt.to.view();

  if (m_to.empty()) {
    std::filesystem::path p = m_from;
    p.replace_extension(".kmp");
    m_to = p;
  }
  if (!FS_TRY(rsl::filesystem::exists(m_from))) {
    fmt::print(stderr, "Error: File {} does not exist.\n", m_from.string());
    return std::unexpected("FolderNotExist");
  }
  if (FS_TRY(rsl::filesystem::exists(m_to))) {
    fmt::print(stderr,
               "Warning: File {} will be overwritten by this operation.\n",
               m_to.string());
  }
  auto file = ReadFile(m_opt.from.view());
  if (!file.has_value()) {
    return std::unexpected("Error: Failed to read file");
  }
  auto kmp = librii::kmp::LoadJSON(
      std::string_view{(char*)file->data(), file->size()});

  oishii::Writer writer(std::endian::big);
  librii::kmp::writeKMP(kmp, writer);
  writer.saveToDisk(m_to.string());
  return {};
}

static Result<void> kcl2json(const CliOptions& m_opt) {
  if (m_opt.verbose) {
    rsl::logging::init();
  }
  std::filesystem::path m_from = m_opt.from.view();
  std::filesystem::path m_to = m_opt.to.view();

  if (m_to.empty()) {
    std::filesystem::path p = m_from;
    p.replace_extension(".json");
    m_to = p;
  }
  if (!FS_TRY(rsl::filesystem::exists(m_from))) {
    fmt::print(stderr, "Error: File {} does not exist.\n", m_from.string());
    return std::unexpected("FolderNotExist");
  }
  if (FS_TRY(rsl::filesystem::exists(m_to))) {
    fmt::print(stderr,
               "Warning: File {} will be overwritten by this operation.\n",
               m_to.string());
  }
  auto file = ReadFile(m_opt.from.view());
  if (!file.has_value()) {
    return std::unexpected("Error: Failed to read file");
  }
  auto kmp = TRY(librii::kcol::ReadKCollisionData(*file, file->size()));

  auto result = DumpJSON(kmp);
  fmt::print("{}\n", result);
  std::ofstream stream(m_to);
  stream << result;

  return {};
}
static Result<void> json2kcl(const CliOptions& m_opt) {
  return std::unexpected("unimplemented!");
}

static Result<void> dumpPresetsG3D(const CliOptions& m_opt) {
  std::filesystem::path m_from = m_opt.from.view();
  std::filesystem::path m_to = m_opt.to.view();

  if (m_to.empty()) {
    std::filesystem::path p = m_from;
    p.replace_extension(".presets");
    m_to = p;
  }
  if (!FS_TRY(rsl::filesystem::exists(m_from))) {
    fmt::print(stderr, "Error: File {} does not exist.\n", m_from.string());
    return std::unexpected("FileNotExist");
  }
  const bool overwrite = FS_TRY(rsl::filesystem::exists(m_to));
  if (overwrite && !FS_TRY(rsl::filesystem::is_directory(m_to))) {
    fmt::print(
        stderr,
        "Error: Cannot write folder to {}--file exists with the same name!\n",
        m_to.string());
    return std::unexpected("NotAFolder");
  }
  if (overwrite) {
    fmt::print(stderr,
               "Warning: Folder {} will be overwritten by this operation.\n",
               m_to.string());
  } else {
    FS_TRY(rsl::filesystem::create_directory(m_to));
  }
  if (m_opt.verbose) {
    rsl::logging::init();
  }
  auto file = ReadFile(m_opt.from.view());
  if (!file.has_value()) {
    return std::unexpected("Error: Failed to read file");
  }

  fmt::print(stderr, "Dumping Presets BRRES,{} => {}\n", m_from.string(),
             m_to.string());

  auto brres = TRY(librii::g3d::Archive::fromFile(m_from.string()));
  TRY(librii::crate::DumpPresetsToFolder(m_to, brres, /*cli*/ true,
                                         /*metadata*/ ""));

  return {};
}
static Result<void> dumpPresetsJ3D(const CliOptions& m_opt) {
  std::filesystem::path m_from = m_opt.from.view();
  std::filesystem::path m_to = m_opt.to.view();

  if (m_to.empty()) {
    std::filesystem::path p = m_from;
    p.replace_extension(".presets");
    m_to = p;
  }
  if (!FS_TRY(rsl::filesystem::exists(m_from))) {
    fmt::print(stderr, "Error: File {} does not exist.\n", m_from.string());
    return std::unexpected("FileNotExist");
  }
  const bool overwrite = FS_TRY(rsl::filesystem::exists(m_to));
  if (overwrite && !FS_TRY(rsl::filesystem::is_directory(m_to))) {
    fmt::print(
        stderr,
        "Error: Cannot write folder to {}--file exists with the same name!\n",
        m_to.string());
    return std::unexpected("NotAFolder");
  }
  if (overwrite) {
    fmt::print(stderr,
               "Warning: Folder {} will be overwritten by this operation.\n",
               m_to.string());
  } else {
    FS_TRY(rsl::filesystem::create_directory(m_to));
  }
  if (m_opt.verbose) {
    rsl::logging::init();
  }
  auto file = ReadFile(m_opt.from.view());
  if (!file.has_value()) {
    return std::unexpected("Error: Failed to read file");
  }

  fmt::print(stderr, "Dumping Presets BMD,{} => {}\n", m_from.string(),
             m_to.string());

  kpi::LightIOTransaction trans;
  trans.callback = [&](kpi::IOMessageClass message_class,
                       const std::string_view domain,
                       const std::string_view message_body) {
    auto msg = std::format("[{}] {} {}", magic_enum::enum_name(message_class),
                           domain, message_body);
    rsl::error(msg);
  };
  auto brres = TRY(librii::j3d::J3dModel::fromFile(m_from.string(), trans));
  TRY(librii::crate::DumpPresetsToFolderJ3D(m_to, brres, /*cli*/ true));

  return {};
}

static Result<void> preciseBmdDump(const CliOptions& m_opt) {
  if (m_opt.verbose) {
    rsl::logging::init();
  }
  std::filesystem::path m_from = m_opt.from.view();
  std::filesystem::path m_to = m_opt.to.view();

  if (m_to.empty()) {
    std::filesystem::path p = m_from;
    p.replace_extension(".obj");
    m_to = p;
  }
  if (!FS_TRY(rsl::filesystem::exists(m_from))) {
    fmt::print(stderr, "Error: File {} does not exist.\n", m_from.string());
    return std::unexpected("FolderNotExist");
  }
  if (FS_TRY(rsl::filesystem::exists(m_to))) {
    fmt::print(stderr,
               "Warning: File {} will be overwritten by this operation.\n",
               m_to.string());
  }
  kpi::LightIOTransaction trans;
  trans.callback =
      [](kpi::IOMessageClass message_class, const std::string_view domain,
         const std::string_view message_body) { rsl::info(message_body); };
  auto bmd = TRY(librii::j3d::J3dModel::fromFile(m_opt.from.view(), trans));

  auto result = TRY(librii::j3d::PreciseBMDDump(bmd, m_opt.from.view()));
  fmt::print("{}\n", result);
  std::ofstream stream(m_to);
  stream << result;

  return {};
}

static Result<void> optimizeG3D(const CliOptions& m_opt) {
  std::filesystem::path m_from = m_opt.from.view();
  std::filesystem::path m_to = m_opt.to.view();

  if (m_to.empty()) {
    m_to = m_from;
  }
  if (!FS_TRY(rsl::filesystem::exists(m_from))) {
    fmt::print(stderr, "Error: File {} does not exist.\n", m_from.string());
    return std::unexpected("FileNotExist");
  }
  const bool overwrite = FS_TRY(rsl::filesystem::exists(m_to));
  if (overwrite && FS_TRY(rsl::filesystem::is_directory(m_to))) {
    fmt::print(
        stderr,
        "Error: Cannot write file to {}--folder exists with the same name!\n",
        m_to.string());
    return std::unexpected("NotAFolder");
  }
  if (overwrite) {
    fmt::print(stderr,
               "Warning: File {} will be overwritten by this operation.\n",
               m_to.string());
  }
  if (m_opt.verbose) {
    rsl::logging::init();
  }
  fmt::print(stderr, "Optimizing BRRES,{} => {}\n", m_from.string(),
             m_to.string());
  auto brres = TRY(librii::g3d::Archive::fromFile(m_from.string()));
  riistudio::g3d::Collection c;
  TRY(riistudio::g3d::ReadBRRES(c, brres, m_from.string()));

  fmt::println(
      "Performing \"facepoint\" reduction strategy (single-core mode)");
  u32 before = 0;
  u32 after = 0;
  rsl::Timer timer;
  timer.reset();
  for (auto& model : c.getModels()) {
    u32 i = 0;
    for (auto& mesh : model.getMeshes()) {
      auto percent =
          static_cast<f32>(i) / static_cast<f32>(model.getMeshes().size());
      ++i;
      progress_put("Optimizing mesh " + mesh.getName(), percent);
      auto rhst = TRY(riistudio::rhst::decompileMesh(mesh, model));
      before += librii::rhst::VertexCount(rhst);
      auto rhstP = TRY(librii::rhst::MeshUtils::TriangulateMesh(rhst));
      for (auto& mp : rhstP.matrix_primitives) {
        TRY(librii::rhst::StripifyTriangles(mp, std::nullopt, mesh.getName(),
                                            m_opt.verbose));
      }
      after += librii::rhst::VertexCount(rhstP);
      TRY(riistudio::rhst::compileMesh(mesh, rhstP, model, false, false));
    }
  }
  float elapsed = static_cast<float>(timer.elapsed()) * 0.001f;
  auto rate =
      100.0 * (1.0f - (static_cast<f64>(after) / static_cast<f64>(before)));
  progress_end();
  fmt::println("Before: {} facepoints; After: {} facepoints", before, after);
  fmt::println("Elapsed time: {:.2f} seconds. Reduction of {:.2f}% "
               "(higher is better)",
               fmt::styled(elapsed, fmt::fg(fmt::color::light_green)),
               fmt::styled(rate, fmt::fg(fmt::color::light_green)));

  auto optimized = c.toLibRii();
  auto d = m_to.string();
  TRY(optimized.write(d));

  return {};
}
static Result<void> optimizeJ3D(const CliOptions& m_opt) {
  std::filesystem::path m_from = m_opt.from.view();
  std::filesystem::path m_to = m_opt.to.view();

  if (m_to.empty()) {
    std::filesystem::path p = m_from;
    p.replace_extension(".bmd");
    m_to = p;
  }
  if (!FS_TRY(rsl::filesystem::exists(m_from))) {
    fmt::print(stderr, "Error: File {} does not exist.\n", m_from.string());
    return std::unexpected("FileNotExist");
  }
  const bool overwrite = FS_TRY(rsl::filesystem::exists(m_to));
  if (overwrite && FS_TRY(rsl::filesystem::is_directory(m_to))) {
    fmt::print(
        stderr,
        "Error: Cannot write file to {}--folder exists with the same name!\n",
        m_to.string());
    return std::unexpected("NotAFolder");
  }
  if (overwrite) {
    fmt::print(stderr,
               "Warning: File {} will be overwritten by this operation.\n",
               m_to.string());
  }
  if (m_opt.verbose) {
    rsl::logging::init();
  }
  fmt::print(stderr, "Optimizing BMD,{} => {}\n", m_from.string(),
             m_to.string());
  auto bmd = TRY(riistudio::j3d::ReadBMD(m_from.string()));
  fmt::println(
      "Performing \"facepoint\" reduction strategy (single-core mode)");
  u32 before = 0;
  u32 after = 0;
  rsl::Timer timer;
  timer.reset();
  for (auto& model : bmd.getModels()) {
    u32 i = 0;
    for (auto& mesh : model.getMeshes()) {
      auto percent =
          static_cast<f32>(i) / static_cast<f32>(model.getMeshes().size());
      ++i;
      progress_put("Optimizing mesh " + mesh.getName(), percent);
      auto rhst = TRY(riistudio::rhst::decompileMesh(mesh, model));
      before += librii::rhst::VertexCount(rhst);
      auto rhstP = TRY(librii::rhst::MeshUtils::TriangulateMesh(rhst));
      for (auto& mp : rhstP.matrix_primitives) {
        TRY(librii::rhst::StripifyTriangles(mp, std::nullopt, mesh.getName(),
                                            m_opt.verbose));
      }
      after += librii::rhst::VertexCount(rhstP);
      TRY(riistudio::rhst::compileMesh(mesh, rhstP, model, false, false));
    }
  }
  float elapsed = static_cast<float>(timer.elapsed()) * 0.001f;
  auto rate =
      100.0 * (1.0f - (static_cast<f64>(after) / static_cast<f64>(before)));
  progress_end();
  fmt::println("Before: {} facepoints; After: {} facepoints", before, after);
  fmt::println("Elapsed time: {:.2f} seconds. Reduction of {:.2f}% "
               "(higher is better)",
               fmt::styled(elapsed, fmt::fg(fmt::color::light_green)),
               fmt::styled(rate, fmt::fg(fmt::color::light_green)));

  oishii::Writer writer(std::endian::big);
  TRY(riistudio::j3d::WriteBMD(bmd, writer, m_opt.verbose));
  writer.saveToDisk(m_to.string());

  return {};
}

static Result<void> import_tex0(const CliOptions& m_opt) {
  if (m_opt.verbose) {
    rsl::logging::init();
  }
  std::filesystem::path m_from = m_opt.from.view();
  std::filesystem::path m_to = m_opt.to.view();

  auto format =
      TRY(rsl::enum_cast<librii::gx::TextureFormat>(m_opt.texture_format));

  if (m_to.empty()) {
    std::filesystem::path p = m_from;
    p.replace_extension(".tex0");
    m_to = p;
  }
  if (!FS_TRY(rsl::filesystem::exists(m_from))) {
    fmt::print(stderr, "Error: File {} does not exist.\n", m_from.string());
    return std::unexpected("FileNotExist");
  }
  if (FS_TRY(rsl::filesystem::exists(m_to))) {
    fmt::print(stderr,
               "Warning: File {} will be overwritten by this operation.\n",
               m_to.string());
  }
  auto path = m_from.string();
  auto image = rsl::stb::load(path);
  if (!image) {
    return std::unexpected(
        std::format("Failed to import texture {}: stb_image didn't "
                    "recognize it or didn't exist.",
                    path));
  }
  std::vector<u8> scratch;
  riistudio::g3d::Texture tex;
  const auto ok = riistudio::rhst::importTexture(
      tex, image->data, scratch, m_opt.mipmaps, m_opt.min_mip, m_opt.max_mips,
      image->width, image->height, image->channels, format);
  if (!ok) {
    return std::unexpected(
        std::format("Failed to import texture {}: {}", path, ok.error()));
  }
  std::filesystem::path fsp(m_opt.from.view());
  tex.setName(fsp.stem().string());

  TRY(librii::g3d::WriteTEX0ToFile(tex, m_to.string()));

  return {};
}

int main(int argc, const char** argv) {
  fmt::print(stdout, "RiiStudio CLI {}\n", RII_TIME_STAMP);
  auto args = parse(argc, argv);
  if (!args) {
    fmt::print("::\n");
    return -1;
  }
  if (args->type == TYPE_KMP2JSON) {
    auto ok = kmp2json(*args);
    if (!ok) {
      fmt::print("{}\n", ok.error());
      return -1;
    }
  }
  if (args->type == TYPE_JSON2KMP) {
    auto ok = json2kmp(*args);
    if (!ok) {
      fmt::print("{}\n", ok.error());
      return -1;
    }
  }

  if (args->type == TYPE_KCL2JSON) {
    auto ok = kcl2json(*args);
    if (!ok) {
      fmt::print("{}\n", ok.error());
      return -1;
    }
  }
  if (args->type == TYPE_JSON2KCL) {
    auto ok = json2kcl(*args);
    if (!ok) {
      fmt::print("{}\n", ok.error());
      return -1;
    }
  }
  if (args->type == TYPE_IMPORT_BRRES) {
    progress_put("Processing...", 0.0f);
    ImportBRRES cmd(*args);
    auto ok = cmd.execute();
    progress_end();
    if (!ok) {
      fmt::print(stderr, "{}\n", ok.error());
      fmt::print(stdout, "{}\n", ok.error());
      return -1;
    }
  }
  if (args->type == TYPE_IMPORT_BMD) {
    progress_put("Processing...", 0.0f);
    ImportBMD cmd(*args);
    auto ok = cmd.execute();
    progress_end();
    if (!ok) {
      fmt::print(stderr, "{}\n", ok.error());
      fmt::print(stdout, "{}\n", ok.error());
      return -1;
    }
  }
  if (args->type == TYPE_DECOMPRESS) {
    DecompressSZS cmd(*args);
    auto ok = cmd.execute();
    if (!ok) {
      fmt::print(stderr, "{}\n", ok.error());
      fmt::print(stdout, "{}\n", ok.error());
      return -1;
    }
  } else if (args->type == TYPE_COMPRESS) {
    CompressSZS cmd(*args);
    auto ok = cmd.execute();
    if (!ok) {
      fmt::print(stderr, "{}\n", ok.error());
      fmt::print(stdout, "{}\n", ok.error());
      return -1;
    }
  } else if (args->type == TYPE_COMPILE_RHST_BRRES) {
    progress_put("Processing...", 0.0f);
    CompileRHST<riistudio::g3d::Collection> cmd(*args);
    auto ok = cmd.execute();
    progress_end();
    if (!ok) {
      fmt::print(stderr, "{}\n", ok.error());
      fmt::print(stdout, "{}\n", ok.error());
      return -1;
    }
  } else if (args->type == TYPE_COMPILE_RHST_BMD) {
    progress_put("Processing...", 0.0f);
    CompileRHST<riistudio::j3d::Collection> cmd(*args);
    auto ok = cmd.execute();
    progress_end();
    if (!ok) {
      fmt::print(stderr, "{}\n", ok.error());
      fmt::print(stdout, "{}\n", ok.error());
      return -1;
    }
  } else if (args->type == TYPE_EXTRACT) {
    ExtractArchive cmd(*args);
    auto ok = cmd.execute();
    if (!ok) {
      fmt::print(stderr, "{}\n", ok.error());
      fmt::print(stdout, "{}\n", ok.error());
      return -1;
    }
  } else if (args->type == TYPE_CREATE) {
    CreateArchive cmd(*args);
    auto ok = cmd.execute();
    if (!ok) {
      fmt::print(stderr, "{}\n", ok.error());
      fmt::print(stdout, "{}\n", ok.error());
      return -1;
    }
  }
  if (args->type == TYPE_DUMP_PRESETS) {
    Result<void> ok = {};
    std::string from_ = rsl::to_lower(args->from.view());
    bool isJ3d = from_.ends_with("bmd") || from_.ends_with("bdl");
    if (isJ3d) {
      ok = dumpPresetsJ3D(*args);
    } else {
      ok = dumpPresetsG3D(*args);
    }
    if (!ok) {
      fmt::print(stderr, "{}\n", ok.error());
      return -1;
    }
  }
  if (args->type == TYPE_PRECISE_BMD_DUMP) {
    auto ok = preciseBmdDump(*args);
    if (!ok) {
      fmt::print(stderr, "{}\n", ok.error());
      return -1;
    }
  }
  if (args->type == TYPE_OPTIMIZE) {
    Result<void> ok = {};
    std::string from_ = rsl::to_lower(args->from.view());
    bool isJ3d = from_.ends_with("bmd") || from_.ends_with("bdl");
    if (isJ3d) {
      ok = optimizeJ3D(*args);
    } else {
      ok = optimizeG3D(*args);
    }
    if (!ok) {
      fmt::print(stderr, "{}\n", ok.error());
      return -1;
    }
  }
  if (args->type == TYPE_IMPORT_TEX0) {
    auto ok = import_tex0(*args);
    if (!ok) {
      fmt::print(stderr, "{}\n", ok.error());
      return -1;
    }
  }
  return 0;
}
