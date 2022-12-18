#include "BoneIO.hpp"

#include <core/util/glm_io.hpp>

namespace librii::g3d {

bool readBone(librii::g3d::BoneData& bone, oishii::BinaryReader& reader,
              u32 bone_id, s32 data_destination) {
  auto start = reader.tell();
  reader.skip(8); // skip size and mdl offset
  bone.mName = readName(reader, data_destination);

  const u32 id = reader.read<u32>();
  if (id != bone_id) {
    return false;
  }
  bone.matrixId = reader.read<u32>();
  const auto bone_flag = reader.read<u32>();
  bone.billboardType = reader.read<u32>();
  reader.read<u32>(); // refId
  bone.mScaling << reader;
  bone.mRotation << reader;
  bone.mTranslation << reader;

  bone.mVolume.min << reader;
  bone.mVolume.max << reader;

  auto readHierarchyElement = [&]() {
    const auto ofs = reader.read<s32>();
    if (ofs == 0)
      return -1;
    // skip to id
    oishii::Jump<oishii::Whence::Set>(reader, data_destination + ofs + 12);
    return static_cast<s32>(reader.read<u32>());
  };
  bone.mParent = readHierarchyElement();
  // Skip sibling and child links -- we recompute it all
  // Skip user data offset
  reader.seekSet(start + 0x70);
  for (auto& d : bone.modelMtx)
    d = reader.read<f32>();
  for (auto& d : bone.inverseModelMtx)
    d = reader.read<f32>();

  setFromFlag(bone, bone_flag);
  return true;
}

void WriteBone(NameTable& names, oishii::Writer& writer, size_t bone_start,
               const BoneData& bone, std::span<const BoneData> bones,
               u32 bone_id, const std::set<s16>& displayMatrices) {
  writeNameForward(names, writer, bone_start, bone.mName);
  writer.write<u32>(bone_id);
  writer.write<u32>(bone.matrixId);
  // TODO: Fix
  writer.write<u32>(computeFlag(bone, bones, displayMatrices.contains(bone.matrixId) || true));
  writer.write<u32>(bone.billboardType);
  writer.write<u32>(0); // TODO: ref
  bone.mScaling >> writer;
  bone.mRotation >> writer;
  bone.mTranslation >> writer;
  bone.mVolume.min >> writer;
  bone.mVolume.max >> writer;

  // Parent, Child, Left, Right
  int parent = bone.mParent;
  int child_first = -1;
  int left = -1;
  int right = -1;

  if (bone.mChildren.size()) {
    child_first = bone.mChildren[0];
  }
  if (bone.mParent != -1) {
    auto& siblings = bones[bone.mParent].mChildren;
    auto it = std::find(siblings.begin(), siblings.end(), bone_id);
    assert(it != siblings.end());
    // Not a cyclic linked list
    left = it == siblings.begin() ? -1 : *(it - 1);
    right = it == siblings.end() - 1 ? -1 : *(it + 1);
  }

  // Assume bones simply increase by 1

  writer.write<s32>(parent == -1 ? 0 : (parent - bone_id) * 0xd0);
  writer.write<s32>(child_first == -1 ? 0 : (child_first - bone_id) * 0xd0);
  writer.write<s32>(right == -1 ? 0 : (right - bone_id) * 0xd0);
  writer.write<s32>(left == -1 ? 0 : (left - bone_id) * 0xd0);

  writer.write<u32>(0); // user data

#if 0
  // Recomputed on runtime?
  // Mtx + Inverse, 3x4 f32
  std::array<f32, 2 * 3 * 4> matrix_data{
      1.0, 0.0,  0.0, 0.0, 0.0,  1.0, 0.0,  0.0, 0.0, 0.0,  1.0, 0.0,
      1.0, -0.0, 0.0, 0.0, -0.0, 1.0, -0.0, 0.0, 0.0, -0.0, 1.0, 0.0};
#endif
  for (auto d : bone.modelMtx)
    writer.write<f32>(d);
  for (auto d : bone.inverseModelMtx)
    writer.write<f32>(d);
}

} // namespace librii::g3d
