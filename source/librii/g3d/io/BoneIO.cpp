#include <core/util/glm_io.hpp>

#include "CommonIO.hpp"
#include "NameTableIO.hpp"
#include <librii/g3d/data/BoneData.hpp>

namespace librii::g3d {

void BinaryBoneData::read(oishii::BinaryReader& reader) {
  auto start = reader.tell();
  reader.skip(8); // skip size and mdl offset
  name = readName(reader, start);

  id = reader.read<u32>();
  matrixId = reader.read<u32>();
  flag = reader.read<u32>();
  billboardType = reader.read<u32>();
  ancestorBillboardBone = reader.read<u32>(); // refId
  scale << reader;
  rotate << reader;
  translate << reader;

  aabb.min << reader;
  aabb.max << reader;

  auto readHierarchyElement = [&]() {
    const auto ofs = reader.read<s32>();
    if (ofs == 0)
      return -1;
    // skip to id
    auto back = reader.tell();
    reader.seekSet(start + ofs + 12);
    auto id = reader.read<s32>();
    reader.seekSet(back);
    return id;
  };
  parent_id = readHierarchyElement();
  child_first_id = readHierarchyElement();
  sibling_right_id = readHierarchyElement();
  sibling_left_id = readHierarchyElement();
  // Skip sibling and child links -- we recompute it all
  // Skip user data offset
  reader.seekSet(start + 0x70);

  // glm matrices are column-major
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 4; ++j)
      modelMtx[j][i] = reader.read<f32>();
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 4; ++j)
      inverseModelMtx[j][i] = reader.read<f32>();
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
