#include "Cli.hpp"
#include <core/util/oishii.hpp>
#include <core/util/timestamp.hpp>
#include <iostream>
#include <librii/assimp2rhst/Assimp.hpp>
#include <librii/assimp2rhst/SupportedFiles.hpp>
#include <plugins/g3d/G3dIo.hpp>
#include <plugins/g3d/collection.hpp>
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

  bool execute() {
    if (m_opt.verbose) {
      rsl::logging::init();
    }
    if (!parseArgs()) {
      fmt::print(stderr, "Error: failed to parse args\n");
      return false;
    }
    if (!librii::assimp2rhst::IsExtensionSupported(m_from.string())) {
      fmt::print(stderr, "Error: file format is unsupported\n");
      return false;
    }
    auto file = OishiiReadFile(m_opt.from);
    if (!file.has_value()) {
      fmt::print(stderr, "Error: Failed to read file\n");
      return false;
    }
    const auto on_log = [](kpi::IOMessageClass c, std::string_view d,
                           std::string_view b) {
      rsl::info("Assimp message: {} {} {}", magic_enum::enum_name(c), d, b);
    };
    auto settings = getSettings();
    auto tree = librii::assimp2rhst::DoImport(m_from.string(), on_log,
                                              file->slice(), settings);
    if (!tree) {
      fmt::print(stderr, "Error: Failed to parse: {}\n", tree.error());
      return false;
    }
    int num_optimized = 0;
    int num_total = 0;
    progress_put(
        std::format("Optimizing meshes {}/{}", num_optimized, num_total), 0.0f);
    auto progress = [&](std::string_view s, float f) {
      progress_put(std::string(s), f);
    };
    auto info = [&](std::string c, std::string v) {
      on_log(kpi::IOMessageClass::Information, c, v);
    };
    auto m_result = std::make_unique<riistudio::g3d::Collection>();
    bool ok = riistudio::rhst::CompileRHST(*tree, *m_result, m_from.string(),
                                           info, progress, m_opt.verbose);
    if (!ok) {
      fmt::print(stderr, "Error: Failed to compile RHST\n");
      return false;
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
    return true;
  }

private:
  bool parseArgs() {
    m_from = std::string{m_opt.from, strnlen(m_opt.from, sizeof(m_opt.from))};
    m_to = std::string{m_opt.to, strnlen(m_opt.to, sizeof(m_opt.to))};
    m_presets =
        std::string{m_opt.preset_path,
                    strnlen(m_opt.preset_path, sizeof(m_opt.preset_path))};
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

int main(int argc, const char** argv) {
  fmt::print(stdout, "RiiStudio CLI {}\n", RII_TIME_STAMP);
  auto args = parse(argc, argv);
  if (!args) {
    return -1;
  }
  progress_put("Processing...", 0.0f);
  ImportBRRES cmd(*args);
  bool ok = cmd.execute();
  progress_end();

  if (!ok) {
    fmt::print(stdout, "\r\nFailed to execute\n");
    fmt::print(stderr, "\r\nFailed to execute\n");
    return -1;
  }

  fmt::print(stdout, "\r\nOK\n");
  return 0;
}
