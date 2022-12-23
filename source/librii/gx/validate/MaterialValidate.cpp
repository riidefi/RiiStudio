#include "MaterialValidate.hpp"

import std.core;

namespace librii::gx {

void expandSharedTexGens(LowLevelGxMaterial& material) {
  // Handled:   2 Sampler 1 Texgen
  // Unhandled: 1 Sampler 2 TexGen

  std::array<u8, 8> texcoord_totaluse{};
  std::array<u8, 8> texmap_totaluse{};

  {
    std::array<std::bitset<8>, 8> texcoords_uses{};
    for (const auto& stage : material.mStages) {
      if (stage.texCoord >= 8) // 0xFF = NULL
        continue;
      const bool is_already_used = texcoords_uses[stage.texCoord][stage.texMap];

      texcoord_totaluse[stage.texCoord] += is_already_used ? 0 : 1;
      texcoords_uses[stage.texCoord].set(stage.texMap);
    }
  }
  {
    std::array<std::bitset<8>, 8> texmaps_uses{};
    for (const auto& stage : material.mStages) {
      if (stage.texMap >= 8) // 0xFF = NULL
        continue;
      const bool is_already_used = texmaps_uses[stage.texMap][stage.texCoord];

      texmap_totaluse[stage.texMap] += is_already_used ? 0 : 1;
      texmaps_uses[stage.texMap].set(stage.texCoord);
    }
  }

  const auto total_texgens =
      std::accumulate(texcoord_totaluse.begin(), texcoord_totaluse.end(), 0);
  if (total_texgens > 8) {
    printf("This setup cannot be expanded! More texgens than available will be "
           "needed\n");
    return;
  }

  if (*std::max_element(texmap_totaluse.begin(), texmap_totaluse.end()) > 1) {
    printf("This setup cannot be expanded! One sampler uses two texgens.\n");
    return;
  }

  // Resize each duplicated slot:
  // (2) (1) (2) -> ((1) (1)) (1) ((1)(1))
  std::array<u8, 8> old_to_new{};
  for (int i = 0; i < material.texGens.size(); ++i)
    old_to_new[i] = i;

  for (int i = 0; i < material.texGens.size(); ++i) {
    if (texcoord_totaluse[i] <= 1)
      continue;

    for (int j = 1; j < texcoord_totaluse[i]; ++j) {
      // Insert new texgen
      material.texGens.push_back(material.texGens[i]);
      std::rotate(material.texGens.begin() + i, material.texGens.end() - 1,
                  material.texGens.end());
    }

    for (int j = i + 1; j < material.texGens.size(); ++j)
      old_to_new[j] += texcoord_totaluse[i] - 1;
  }

  // Remap stage references
  std::array<u8, 8> coord_remaps{};
  std::array<std::bitset<8>, 8> remapped_coord_uses;

  for (auto& stage : material.mStages) {
    const auto coord = stage.texCoord;
    if (coord >= old_to_new.size())
      continue;
    stage.texCoord = old_to_new[coord];

    // One use: nothing to do
    if (texcoord_totaluse[coord] <= 1)
      continue;

    // Only handle each coord[map] once
    if (remapped_coord_uses[coord][stage.texMap])
      continue;
    remapped_coord_uses[coord][stage.texMap] = true;

    ++coord_remaps[coord];
    const auto delta = coord_remaps[coord] - 1;
    stage.texCoord += delta;
  }
}

} // namespace librii::gx
