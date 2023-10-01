#pragma once

namespace librii::jpa {

struct JPAExTexBlock {
  IndTextureMode indTextureMode;
  glm::mat2x4 indTextureMtx;
  u8 indTextureID;
  u8 subTextureID;
  u8 secondTextureIndex;
};

} // namespace librii::jpa
