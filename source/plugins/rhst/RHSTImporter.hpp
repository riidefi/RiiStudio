#pragma once

#include <core/kpi/Plugins.hpp>
#include <librii/rhst/RHST.hpp>

namespace riistudio::rhst {

[[nodiscard]] Result<void> importTexture(libcube::Texture& data, u8* image,
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

} // namespace riistudio::rhst
