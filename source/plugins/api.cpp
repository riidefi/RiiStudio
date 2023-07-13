#include <LibBadUIFramework/Node2.hpp>
#include <LibBadUIFramework/PropertyView.hpp>
#include <LibBadUIFramework/RichNameManager.hpp>
#include <core/common.h>
#include <oishii/reader/binary_reader.hxx>

#include <memory>
#include <vector>

extern void InstallGC();

namespace riistudio::g3d {
void InstallG3d();
}

namespace riistudio::j3d {
void InstallJ3d();
}

kpi::RichName GetRich(const std::string& type) {
  const auto* got = kpi::ReflectionMesh::getInstance()->getDataMesh().get(type);
  assert(got);
  return got->mName;
}

void InitAPI() {
  InstallGC();

  riistudio::g3d::InstallG3d();
  riistudio::j3d::InstallJ3d();

  kpi::ReflectionMesh::getInstance()->getDataMesh().compute();
}
void DeinitAPI() {}
