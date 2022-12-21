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
    oishii::Jump<oishii::Whence::Set>(reader, start + ofs + 12);
    return static_cast<s32>(reader.read<u32>());
  };
  parent_id = readHierarchyElement();
  child_first_id = readHierarchyElement();
  sibling_right_id = readHierarchyElement();
  sibling_left_id = readHierarchyElement();
  // Skip sibling and child links -- we recompute it all
  // Skip user data offset
  reader.seekSet(start + 0x70);
  for (auto& d : modelMtx)
    d = reader.read<f32>();
  for (auto& d : inverseModelMtx)
    d = reader.read<f32>();
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

  for (auto d : modelMtx)
    writer.write<f32>(d);
  for (auto d : inverseModelMtx)
    writer.write<f32>(d);
}

} // namespace librii::g3d
