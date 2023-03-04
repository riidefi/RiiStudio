#pragma once

#include <LibBadUIFramework/Plugins.hpp>
#include <librii/image/ImagePlatform.hpp>
#include <librii/rhst/RHST.hpp>
#include <plugins/gc/Export/Scene.hpp>
#include <plugins/gc/Export/Texture.hpp>

namespace riistudio::rhst {

[[nodiscard]] Result<void>
importTextureImpl(libcube::Texture& data, std::span<u8> image,
                  std::vector<u8>& scratch, int num_mip, int width, int height,
                  int first_w, int first_h, librii::gx::TextureFormat fmt,
                  librii::image::ResizingAlgorithm resize =
                      librii::image::ResizingAlgorithm::Lanczos);

[[nodiscard]] Result<void> importTexture(libcube::Texture& data,
                                         std::span<u8> image,
                                         std::vector<u8>& scratch, bool mip_gen,
                                         int min_dim, int max_mip, int width,
                                         int height, int channels);
[[nodiscard]] Result<void> importTextureFromMemory(libcube::Texture& data,
                                                   std::span<const u8> span,
                                                   std::vector<u8>& scratch,
                                                   bool mip_gen, int min_dim,
                                                   int max_mip);
[[nodiscard]] Result<void> importTextureFromFile(libcube::Texture& data,
                                                 std::string_view path,
                                                 std::vector<u8>& scratch,
                                                 bool mip_gen, int min_dim,
                                                 int max_mip);

void CompileRHST(librii::rhst::SceneTree& rhst,
                 kpi::IOTransaction& transaction);

[[nodiscard]] bool CompileRHST(
    librii::rhst::SceneTree& rhst, libcube::Scene& scene, std::string path,
    std::function<void(std::string, std::string)> info,
    std::function<void(std::string_view, float)> progress, bool verbose = true);

[[nodiscard]] Result<librii::rhst::Mesh>
decompileMesh(const libcube::IndexedPolygon& src, const libcube::Model& mdl);

[[nodiscard]] Result<void> compileMesh(libcube::IndexedPolygon& dst,
                                       const librii::rhst::Mesh& src,
                                       libcube::Model& model,
                                       bool optimize = true,
                                       bool reinit_bufs = true);

} // namespace riistudio::rhst
