#include "Scene.hpp"
#include "io/BMD.hpp"

namespace riistudio::j3d {


kpi::DecentralizedInstaller Installer([](kpi::ApplicationPlugins& installer) {
  installer.addType<j3d::Model, lib3d::Model>();
  installer.addType<j3d::Collection, lib3d::Scene>();

  installer.addType<j3d::Texture, libcube::Texture>();
  installer.addType<j3d::Material, libcube::IGCMaterial>();
  installer.addType<j3d::Shape, libcube::IndexedPolygon>();
  installer.addType<j3d::Joint, libcube::IBoneDelegate>();
});

} // namespace riistudio::j3d
