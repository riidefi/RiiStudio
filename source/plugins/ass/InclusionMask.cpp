#include "InclusionMask.hpp"

#include <stdio.h>
#include <vendor/assimp/config.h>

IMPORT_STD;

namespace riistudio::ass {

void DebugPrintExclusionMask(u32 exclusion_mask) {
  if (exclusion_mask & aiComponent_NORMALS)
    puts("Excluding normals");
  if (exclusion_mask & aiComponent_TANGENTS_AND_BITANGENTS)
    puts("Excluding tangents/bitangents");
  if (exclusion_mask & aiComponent_COLORS)
    puts("Excluding colors");
  if (exclusion_mask & aiComponent_TEXCOORDS)
    puts("Excluding uvs");
  if (exclusion_mask & aiComponent_BONEWEIGHTS)
    puts("Excluding boneweights");
  if (exclusion_mask & aiComponent_ANIMATIONS)
    puts("Excluding animations");
  if (exclusion_mask & aiComponent_TEXTURES)
    puts("Excluding textures");
  if (exclusion_mask & aiComponent_LIGHTS)
    puts("Excluding lights");
  if (exclusion_mask & aiComponent_CAMERAS)
    puts("Excluding cameras");
  if (exclusion_mask & aiComponent_MESHES)
    puts("Excluding meshes");
  if (exclusion_mask & aiComponent_MATERIALS)
    puts("Excluding materials");
}

u32 FlipExclusionMask(u32 exclusion_mask) {
  return (~exclusion_mask) & ((aiComponent_MATERIALS << 1) - 1);
}

u32 DefaultInclusionMask() {
  return aiComponent_NORMALS | aiComponent_COLORS | aiComponent_TEXCOORDS |
         aiComponent_TEXTURES | aiComponent_MESHES | aiComponent_MATERIALS;
}

} // namespace riistudio::ass
