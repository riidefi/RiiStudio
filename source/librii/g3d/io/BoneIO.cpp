#include <core/util/glm_io.hpp>

#include "CommonIO.hpp"
#include "NameTableIO.hpp"
#include <librii/g3d/data/BoneData.hpp>
#include <rsl/SafeReader.hpp>

namespace librii::g3d {

Result<glm::vec3> readVec3(rsl::SafeReader& safe) {
  auto x = TRY(safe.F32());
  auto y = TRY(safe.F32());
  auto z = TRY(safe.F32());
  return glm::vec3(x, y, z);
}

Result<void> BinaryBoneData::read(oishii::BinaryReader& unsafeReader) {
  rsl::SafeReader reader(unsafeReader);
  auto start = reader.tell();
  unsafeReader.skip(8); // skip size and mdl offset
  name = TRY(reader.StringOfs(start));

  id = TRY(reader.U32());
  matrixId = TRY(reader.U32());
  DebugPrint("matrixId = {}", matrixId);
  flag = TRY(reader.U32());
  billboardType = TRY(reader.U32());
  ancestorBillboardBone = TRY(reader.U32()); // refId
  scale = TRY(readVec3(reader));
  rotate = TRY(readVec3(reader));
  translate = TRY(readVec3(reader));

  aabb.min = TRY(readVec3(reader));
  aabb.max = TRY(readVec3(reader));

  auto readHierarchyElement = [&]() -> Result<s32> {
    const auto ofs = TRY(reader.S32());
    if (ofs == 0)
      return -1;
    // skip to id
    auto back = reader.tell();
    reader.seekSet(start + ofs + 12);
    auto id = TRY(reader.S32());
    reader.seekSet(back);
    return id;
  };
  parent_id = TRY(readHierarchyElement());
  child_first_id = TRY(readHierarchyElement());
  sibling_right_id = TRY(readHierarchyElement());
  sibling_left_id = TRY(readHierarchyElement());
  // Skip sibling and child links -- we recompute it all
  // Skip user data offset
  reader.seekSet(start + 0x70);

  // glm matrices are column-major
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 4; ++j)
      modelMtx[j][i] = TRY(reader.F32());
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 4; ++j)
      inverseModelMtx[j][i] = TRY(reader.F32());
  return {};
}
void BinaryBoneData::write(NameTable& names, oishii::Writer& writer,
                           u32 mdl_start) const {
  auto bone_start = writer.tell();
  writer.write<u32>(0xD0);
  writer.write<s32>(mdl_start - bone_start);
  writeNameForward(names, writer, bone_start, name);
  writer.write<u32>(id);
  writer.write<u32>(matrixId);
  writer.write<u32>(flag);
  writer.write<u32>(billboardType);
  writer.write<u32>(ancestorBillboardBone);
  scale >> writer;
  rotate >> writer;
  translate >> writer;
  aabb.min >> writer;
  aabb.max >> writer;

  // Assume bones simply increase by 1 (ID = Index)
  writer.write<s32>(parent_id == -1 ? 0 : (parent_id - id) * 0xd0);
  writer.write<s32>(child_first_id == -1 ? 0 : (child_first_id - id) * 0xd0);
  writer.write<s32>(sibling_right_id == -1 ? 0
                                           : (sibling_right_id - id) * 0xd0);
  writer.write<s32>(sibling_left_id == -1 ? 0 : (sibling_left_id - id) * 0xd0);

  writer.write<u32>(0); // user data

  // glm matrices are column-major
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 4; ++j)
      writer.write<f32>(modelMtx[j][i]);
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 4; ++j)
      writer.write<f32>(inverseModelMtx[j][i]);
}

} // namespace librii::g3d
