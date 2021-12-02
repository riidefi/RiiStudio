#pragma once

#include <core/common.h>
#include <plugins/gc/Export/Texture.hpp>
#include <vector>	

namespace riistudio::ass {

bool importTexture(libcube::Texture& data, u8* image, std::vector<u8>& scratch,
                   bool mip_gen, int min_dim, int max_mip, int width,
                   int height, int channels);
bool importTexture(libcube::Texture& data, const u8* idata, const u32 isize,
                   std::vector<u8>& scratch, bool mip_gen, int min_dim,
                   int max_mip);
bool importTexture(libcube::Texture& data, const char* path,
                   std::vector<u8>& scratch, bool mip_gen, int min_dim,
                   int max_mip);

} // namespace riistudio::ass
