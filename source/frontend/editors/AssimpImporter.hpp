#pragma once

#include <frontend/legacy_editor/StudioWindow.hpp>
#include <imcxx/Widgets.hpp>
#include <librii/assimp2rhst/Assimp.hpp>
#include <librii/assimp2rhst/SupportedFiles.hpp>
#include <plugins/rhst/RHSTImporter.hpp>
#include <rsl/FsDialog.hpp>

#include <future>
#include <plugins/g3d/collection.hpp>
#include <plugins/j3d/Scene.hpp>

namespace riistudio::frontend {

class AssimpEditorPropertyGrid {
public:
  void Draw(librii::assimp2rhst::Settings& x);
};

class AssimpImporter : public frontend::StudioWindow {
public:
  AssimpImporter(std::span<const u8> buf, std::string_view path)
      : StudioWindow("Assimp Importer: <unknown>", DockSetting::None),
        m_data(buf.begin(), buf.end()) {
    m_path = path;
    setName("Assimp Importer: " + std::string(m_path));
  }
  AssimpImporter(const AssimpImporter&) = delete;
  AssimpImporter(AssimpImporter&&) = delete;

  static bool supports(std::string_view path) {
    return librii::assimp2rhst::IsExtensionSupported(path);
  }

  void draw_() override;

private:
  enum class State {
    PickFormat,
    Settings,
    Wait,
    Done,
    Fail,
  };
  struct Status {
    std::string tagline = "Waiting";
    float progress = 0.0f;
  };

  void launchImporter() {
    m_importerFuture = std::async(std::launch::async, [&]() {
      const auto on_log = [](kpi::IOMessageClass c, std::string_view d,
                             std::string_view b) {
        rsl::info("Assimp message: {} {} {}", magic_enum::enum_name(c), d, b);
      };
      importerThreadSetStatus(std::format("Parsing .dae/.fbx file"), 0.0f);
      auto tree =
          librii::assimp2rhst::DoImport(m_path, on_log, m_data, m_settings);
      if (!tree) {
        m_err = "Failed to parse: " + tree.error();
        m_state = State::Fail;
      }
      int num_optimized = 0;
      int num_total = 0;
      importerThreadSetStatus(
          std::format("Optimizing meshes {}/{}", num_optimized, num_total),
          0.0f);
      auto progress = [&](std::string_view s, float f) {
        importerThreadSetStatus(std::string(s), f);
      };
      auto info = [&](std::string c, std::string v) {
        on_log(kpi::IOMessageClass::Information, c, v);
      };
      auto mips = getMips();
      bool ok = riistudio::rhst::CompileRHST(*tree, *m_result, m_path, info,
                                             progress, mips);
      // We can trust the UI thread will not write to this when in ::Waiting
      if (!ok) {
        m_err = "Failed to convert to BRRES/BMD.";
      }
      m_state = ok ? State::Done : State::Fail;
    });
  }

  // Importer thread only
  void importerThreadSetStatus(std::string&& tagline, float progress) {
    std::unique_lock g(m_statusLock);
    m_status.tagline = std::move(tagline);
    m_status.progress = progress;
  }
  // UI thead only
  Status uiThreadGetStatus() {
    std::unique_lock g(m_statusLock);
    return m_status;
  }

  auto getMips() const -> std::optional<riistudio::rhst::MipGen> {
    if (!m_settings.mGenerateMipMaps) {
      return std::nullopt;
    }
    return riistudio::rhst::MipGen{
        .min_dim = static_cast<u32>(m_settings.mMinMipDimension),
        .max_mip = static_cast<u32>(m_settings.mMaxMipCount),
    };
  }

  AssimpEditorPropertyGrid m_grid;
  std::string m_path;
  librii::assimp2rhst::Settings m_settings;
  State m_state = State::PickFormat;
  std::unique_ptr<libcube::Scene> m_result = nullptr;
  // .brres, .bmd
  std::string m_extension;
  std::future<void> m_importerFuture;
  std::vector<u8> m_data;

  // WRITE-ONLY for importer task
  // READ-ONLY for UI
  std::mutex m_statusLock;
  Status m_status;
  std::string m_err;
};

} // namespace riistudio::frontend
