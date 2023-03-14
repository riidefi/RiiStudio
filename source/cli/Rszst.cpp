#include "Cli.hpp"
#include <core/util/oishii.hpp>
#include <core/util/timestamp.hpp>
#include <iostream>
#include <librii/assimp/LRAssimp.hpp>
#include <librii/assimp2rhst/Assimp.hpp>
#include <librii/assimp2rhst/SupportedFiles.hpp>
#include <librii/szs/SZS.hpp>
#include <librii/u8/U8.hpp>
#include <plugins/g3d/G3dIo.hpp>
#include <plugins/g3d/collection.hpp>
#include <plugins/j3d/J3dIo.hpp>
#include <plugins/rhst/RHSTImporter.hpp>
#include <sstream>

namespace riistudio {
const char* translateString(std::string_view str) { return str.data(); }
} // namespace riistudio

namespace llvm {
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

class ImportBRRES {
public:
  ImportBRRES(const CliOptions& opt) : m_opt(opt) {}

  Result<void> execute() {
    if (m_opt.verbose) {
      rsl::logging::init();
    }
    if (!parseArgs()) {
      return std::unexpected("Failed to parse args");
    }
    if (!librii::assimp2rhst::IsExtensionSupported(m_from.string())) {
      return std::unexpected("File format is unsupported");
    }
    auto file = OishiiReadFile(m_opt.from.view());
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
      auto* pScene =
          ReadScene(on_log, file->slice(), m_from.string(), settings, importer);
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
                                              file->slice(), settings);
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
                                           info, progress, !m_opt.no_tristrip,
                                           m_opt.verbose);
    if (!ok) {
      return std::unexpected("Failed to parse RHST");
    }
    std::unordered_map<std::string, librii::crate::CrateAnimation> presets;
    if (std::filesystem::exists(m_presets) &&
        std::filesystem::is_directory(m_presets)) {
      for (auto it : std::filesystem::directory_iterator(m_presets)) {
        if (it.path().extension() == ".rspreset") {
          auto file = OishiiReadFile(it.path().string());
          if (!file) {
            fmt::print(stderr, "Failed to read rspreset: {}\n",
                       it.path().string());
            continue;
          }
          auto preset = librii::crate::ReadRSPreset(file->slice());
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
    oishii::Writer result(0);
    riistudio::g3d::WriteBRRES(*m_result, result);
    OishiiFlushWriter(result, m_to.string());
    return {};
  }

private:
  bool parseArgs() {
    m_from = m_opt.from.view();
    m_to = m_opt.to.view();
    m_presets = m_opt.preset_path.view();
    if (m_to.empty()) {
      std::filesystem::path p = m_from;
      p.replace_extension(".brres");
      m_to = p;
    }
    if (!std::filesystem::exists(m_from)) {
      fmt::print(stderr, "Error: File {} does not exist.\n", m_from.string());
      return false;
    }
    if (std::filesystem::exists(m_to)) {
      fmt::print(stderr,
                 "Warning: File {} will be overwritten by this operation.\n",
                 m_to.string());
    }
    if (!m_presets.empty()) {
      if (!std::filesystem::exists(m_presets)) {
        fmt::print(stderr,
                   "Warning: Preset path {} does not exist not valid.\n",
                   m_presets.string());
        m_presets.clear();
      } else if (!std::filesystem::is_directory(m_presets)) {
        fmt::print(stderr,
                   "Warning: Preset path {} does not refer to a folder.\n",
                   m_presets.string());
        m_presets.clear();
      } else {
        fmt::print(stdout, "Info: Reading presets from {}\n",
                   m_presets.string());
      }
    }
    return true;
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
    if (!parseArgs()) {
      return std::unexpected("Error: failed to parse args");
    }
    fmt::print(stderr, "Decompressing SZS: {} => {}\n", m_from.string(),
               m_to.string());
    auto file = OishiiReadFile(m_opt.from.view());
    if (!file.has_value()) {
      return std::unexpected("Error: Failed to read file");
    }
    // Max 4GB
    u32 size = TRY(librii::szs::getExpandedSize(file->slice()));
    std::vector<u8> buf(size);
    TRY(librii::szs::decode(buf, file->slice()));
    if (std::filesystem::is_directory(m_to)) {
      return std::unexpected("Failed to extract: |to| is a folder, not a file");
    }
    plate::Platform::writeFile(buf, m_to.string());
    return {};
  }

private:
  bool parseArgs() {
    m_from = std::string{m_opt.from.view()};
    m_to = std::string{m_opt.to.view()};

    if (m_to.empty()) {
      std::filesystem::path p = m_from;
      p.replace_extension(".arc");
      m_to = p;
    }
    if (!std::filesystem::exists(m_from)) {
      fmt::print(stderr, "Error: File {} does not exist.\n", m_from.string());
      return false;
    }
    if (std::filesystem::exists(m_to)) {
      fmt::print(stderr,
                 "Warning: Folder {} will be overwritten by this operation.\n",
                 m_to.string());
    }
    return true;
  }

  CliOptions m_opt;
  std::filesystem::path m_from;
  std::filesystem::path m_to;
};
class CompressSZS {
public:
  CompressSZS(const CliOptions& opt) : m_opt(opt) {}

  bool execute() {
    if (m_opt.verbose) {
      rsl::logging::init();
    }
    if (!parseArgs()) {
      fmt::print(stderr, "Error: failed to parse args\n");
      return false;
    }
    if (false) {
      fmt::print(stderr, "Error: file format is unsupported\n");
      return false;
    }
    auto from = std::filesystem::absolute(m_from);
    auto file = OishiiReadFile(from.string());
    if (!file.has_value()) {
      fmt::print(stderr, "Error: Failed to read file {}\n", from.string());
      return false;
    }

    fmt::print(stderr,
               "Compressing SZS: {} => {} (Boyer-Moore-Horspool strategy)\n",
               m_from.string(), m_to.string());
    std::vector<u8> buf(file->slice().size_bytes() * 2);
    int size = librii::szs::encodeBoyerMooreHorspool(
        file->slice().data(), buf.data(), file->slice().size_bytes());
    if (size < 0 || size > buf.size()) {
      fmt::print(stderr, "Error: Failed to compress file\n");
      return false;
    }
    buf.resize(roundUp(size, 32));

    plate::Platform::writeFile(buf, m_to.string());
    return true;
  }

private:
  bool parseArgs() {
    m_from = m_opt.from.view();
    m_to = m_opt.to.view();

    if (m_to.empty()) {
      std::filesystem::path p = m_from;
      p.replace_extension(".szs");
      m_to = p;
    }
    if (!std::filesystem::exists(m_from)) {
      fmt::print(stderr, "Error: File {} does not exist.\n", m_from.string());
      return false;
    }
    if (std::filesystem::exists(m_to)) {
      fmt::print(stderr,
                 "Warning: File {} will be overwritten by this operation.\n",
                 m_to.string());
    }
    return true;
  }

  CliOptions m_opt;
  std::filesystem::path m_from;
  std::filesystem::path m_to;
};

void WriteIt(riistudio::g3d::Collection& c, oishii::Writer& w) {
  riistudio::g3d::WriteBRRES(c, w);
}
std::string ExOf(riistudio::g3d::Collection* c) { return ".brres"; }
void WriteIt(riistudio::j3d::Collection& c, oishii::Writer& w) {
  riistudio::j3d::WriteBMD(c, w);
}
std::string ExOf(riistudio::j3d::Collection* c) { return ".bmd"; }

template <typename T> class CompileRHST {
public:
  CompileRHST(const CliOptions& opt) : m_opt(opt) {}

  bool execute() {
    if (m_opt.verbose) {
      rsl::logging::init();
    }
    if (!parseArgs()) {
      fmt::print(stderr, "Error: failed to parse args\n");
      return false;
    }
    if (m_from.extension() != ".rhst") {
      fmt::print(stderr, "Error: file format is unsupported\n");
      return false;
    }
    auto file = OishiiReadFile(m_opt.from.view());
    if (!file.has_value()) {
      fmt::print(stderr, "Error: Failed to read file\n");
      return false;
    }
    auto tree = librii::rhst::ReadSceneTree(file->slice());
    if (!tree) {
      fmt::print(stderr, "Error: Failed to parse: {}\n", tree.error());
      return false;
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
                                           info, progress, !m_opt.no_tristrip,
                                           m_opt.verbose);
    if (!ok) {
      fmt::print(stderr, "Error: Failed to compile RHST\n");
      return false;
    }
    oishii::Writer result(0);
    WriteIt(*m_result, result);
    OishiiFlushWriter(result, m_to.string());
    return true;
  }

private:
  bool parseArgs() {
    m_from = m_opt.from.view();
    m_to = m_opt.to.view();
    if (m_to.empty()) {
      std::filesystem::path p = m_from;
      p.replace_extension(ExOf((T*)nullptr));
      m_to = p;
    }
    if (!std::filesystem::exists(m_from)) {
      fmt::print(stderr, "Error: File {} does not exist.\n", m_from.string());
      return false;
    }
    if (std::filesystem::exists(m_to)) {
      fmt::print(stderr,
                 "Warning: File {} will be overwritten by this operation.\n",
                 m_to.string());
    }
    return true;
  }

  CliOptions m_opt;
  std::filesystem::path m_from;
  std::filesystem::path m_to;
};

class ExtractSZS {
public:
  ExtractSZS(const CliOptions& opt) : m_opt(opt) {}

  Result<void> execute() {
    if (m_opt.verbose) {
      rsl::logging::init();
    }
    if (!parseArgs()) {
      return std::unexpected("Error: failed to parse args");
    }
    auto file = OishiiReadFile(m_opt.from.view());
    if (!file.has_value()) {
      return std::unexpected("Error: Failed to read file");
    }

    // Max 4GB
    u32 size = TRY(librii::szs::getExpandedSize(file->slice()));
    std::vector<u8> buf(size);
    TRY(librii::szs::decode(buf, file->slice()));

    fmt::print(stderr, "Extracting ARC.SZS,{} => {}\n", m_from.string(),
               m_to.string());

    auto arc = TRY(librii::U8::LoadU8Archive(buf));
    TRY(librii::U8::Extract(arc, m_to));

    return {};
  }

private:
  bool parseArgs() {
    m_from = m_opt.from.view();
    m_to = m_opt.to.view();

    if (m_to.empty()) {
      std::filesystem::path p = m_from;
      p.replace_extension(".szs");
      m_to = p;
    }
    if (!std::filesystem::exists(m_from)) {
      fmt::print(stderr, "Error: File {} does not exist.\n", m_from.string());
      return false;
    }
    if (std::filesystem::exists(m_to)) {
      fmt::print(stderr,
                 "Warning: File {} will be overwritten by this operation.\n",
                 m_to.string());
    }
    return true;
  }

  CliOptions m_opt;
  std::filesystem::path m_from;
  std::filesystem::path m_to;
};

class CreateSZS {
public:
  CreateSZS(const CliOptions& opt) : m_opt(opt) {}

  Result<void> execute() {
    if (m_opt.verbose) {
      rsl::logging::init();
    }
    if (!parseArgs()) {
      return std::unexpected("Error: failed to parse args");
    }
    if (!std::filesystem::is_directory(m_from)) {
      return std::unexpected("Expected a folder, not a file as input");
    }
    fmt::print(stderr, "Creating ARC.SZS,{} => {}\n", m_from.string(),
               std::filesystem::absolute(m_to).string());

    auto arc = TRY(librii::U8::Create(m_from));
    auto buf = librii::U8::SaveU8Archive(arc);
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

    plate::Platform::writeFile(szs, m_to.string());

    return {};
  }

private:
  bool parseArgs() {
    m_from = m_opt.from.view();
    m_to = m_opt.to.view();

    if (m_to.empty()) {
      std::filesystem::path p = m_from;
      p.replace_extension(".d");
      m_to = p;
    }
    if (!std::filesystem::exists(m_from)) {
      fmt::print(stderr, "Error: File {} does not exist.\n", m_from.string());
      return false;
    }
    if (std::filesystem::exists(m_to)) {
      fmt::print(stderr,
                 "Warning: File {} will be overwritten by this operation.\n",
                 m_to.string());
    }
    return true;
  }

  CliOptions m_opt;
  std::filesystem::path m_from;
  std::filesystem::path m_to;
};

int main(int argc, const char** argv) {
  fmt::print(stdout, "RiiStudio CLI {}\n", RII_TIME_STAMP);
  auto args = parse(argc, argv);
  if (!args) {
    fmt::print("::\n");
    return -1;
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
  } else if (args->type == TYPE_DECOMPRESS) {
    DecompressSZS cmd(*args);
    auto ok = cmd.execute();
    if (!ok) {
      fmt::print(stderr, "{}\n", ok.error());
      fmt::print(stdout, "{}\n", ok.error());
      return -1;
    }
  } else if (args->type == TYPE_COMPRESS) {
    CompressSZS cmd(*args);
    bool ok = cmd.execute();
    if (!ok) {
      fmt::print(stdout, "\r\nFailed to execute\n");
      fmt::print(stderr, "\r\nFailed to execute\n");
      return -1;
    }
  } else if (args->type == TYPE_COMPILE_RHST_BRRES) {
    progress_put("Processing...", 0.0f);
    CompileRHST<riistudio::g3d::Collection> cmd(*args);
    bool ok = cmd.execute();
    progress_end();
  } else if (args->type == TYPE_COMPILE_RHST_BMD) {
    progress_put("Processing...", 0.0f);
    CompileRHST<riistudio::j3d::Collection> cmd(*args);
    bool ok = cmd.execute();
    progress_end();
  } else if (args->type == TYPE_EXTRACT) {
    ExtractSZS cmd(*args);
    auto ok = cmd.execute();
    if (!ok) {
      fmt::print(stderr, "{}\n", ok.error());
      fmt::print(stdout, "{}\n", ok.error());
      return -1;
    }
  } else if (args->type == TYPE_CREATE) {
    CreateSZS cmd(*args);
    auto ok = cmd.execute();
    if (!ok) {
      fmt::print(stderr, "{}\n", ok.error());
      fmt::print(stdout, "{}\n", ok.error());
      return -1;
    }
  }
  return 0;
}
