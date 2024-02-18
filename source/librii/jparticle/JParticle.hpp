#pragma once


#include "oishii/reader/binary_reader.hxx"
#include "Sections/JPABaseShapeBlock.hpp"
#include "Sections/JPADynamicsBlock.hpp"
#include "Sections/JPAFieldBlock.hpp"
#include "Sections/JPAExtraShapeBlock.hpp"
#include "Sections/JPAChildShapeBlock.hpp"
#include "Sections/JPAKeyBlock.hpp"
#include "Sections/JPAExTexBlock.hpp"
#include "Sections/TextureBlock.hpp"
#include "plugins/j3d/Texture.hpp"

#include <array>
#include <core/common.h>

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <librii/math/aabb.hpp>
#include <string>
#include <vector>


namespace librii::jpa {



struct JPAResource {
  JPADynamicsBlock bem1;
  JPABaseShapeBlock bsp1;
  std::optional<JPAExtraShapeBlock> esp1;
  std::optional<JPAExTexBlock> etx1;
  std::optional<JPAChildShapeBlock> ssp1;
  std::vector<JPAFieldBlock> fld1;
  std::vector<JPAKeyBlock> kfa1;
  std::vector<u16> tdb1;
};


/// Abstracted strcuture
struct JPAC {
  u32 version;
  u32 blockCount;

  std::vector<JPAResource> resources;
  std::vector<TextureBlock> textures;


  bool operator==(const JPAC&) const = default;

  Result<JPAC> loadFromFile(std::string_view fileName);
  Result<JPAC> loadFromMemory(std::span<const u8> buf,
                              std::string_view filename);
  Result<JPAC> loadFromStream(oishii::BinaryReader& reader);

  static Result<JPAC> fromFile(std::string_view path);
  static Result<JPAC> fromMemory(std::span<const u8> buf,
                                 std::string_view path);

  Result<void> load_block_data_from_file(oishii::BinaryReader& reader,
                                       s32 tag_count);
  Result<void> load_block_data_from_file_JPAC2_10(oishii::BinaryReader& reader,
                                                  s32 block_count);

};

} // namespace librii::j3d
