#include "Cli.hpp"
#include <core/util/oishii.hpp>
#include <core/util/timestamp.hpp>
#include <iostream>
#include <librii/assimp2rhst/Assimp.hpp>
#include <librii/assimp2rhst/SupportedFiles.hpp>
#include <plugins/g3d/G3dIo.hpp>
#include <plugins/g3d/collection.hpp>
#include <plugins/rhst/RHSTImporter.hpp>

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

static void progress_put(std::string status, float percent, int barWidth = 70) {
  std::cout << status << " [";
  int pos = static_cast<int>(static_cast<float>(barWidth) * percent);
  for (int i = 0; i < barWidth; ++i) {
    if (i < pos)
      std::cout << "=";
    else if (i == pos)
      std::cout << ">";
    else
      std::cout << " ";
  }
  std::cout << "] " << static_cast<int>(percent * 100.0f) << " %"
            << "\r";
  std::cout.flush();
}
static void progress_end() { std::cout << std::endl; }

class ImportBRRES {
public:
  ImportBRRES(const CliOptions& opt) : m_opt(opt) {}

  bool execute() {
    if (!parseArgs()) {
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
      fmt::print(stderr, "Error: Failed to compile RHST");
      return false;
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
    if (m_to.empty()) {
      std::filesystem::path p = m_from;
      p.replace_extension(".brres");
      m_to = p;
    }
    if (!std::filesystem::exists(m_from)) {
      std::cerr << std::format("Error: File {} does not exist.",
                               m_from.string())
                << std::endl;
      return false;
    }
    if (std::filesystem::exists(m_to)) {
      std::cerr
          << std::format(
                 "Warning: File {} will be overwritten by this operation.",
                 m_to.string())
          << std::endl;
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
};

int main(int argc, const char** argv) {
  printf("RiiStudio CLI %s\n", RII_TIME_STAMP);
  auto args = parse(argc, argv);
  if (!args) {
    return -1;
  }
  progress_put("Processing...", 0.0f);
  ImportBRRES cmd(*args);
  bool ok = cmd.execute();
  progress_end();

  return ok ? 0 : -1;
}
