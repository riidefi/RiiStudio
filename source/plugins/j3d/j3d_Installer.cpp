#include "Installer.hpp"

#include "Scene.hpp"
#include "io/BMD.hpp"

namespace riistudio::j3d {

namespace ui {
void install();
}

void Install(kpi::ApplicationPlugins& installer) {
  InstallBMD(installer);

  installer.addType<j3d::Model, lib3d::Model>();
  installer.addType<j3d::Collection, lib3d::Scene>();

  installer.addType<j3d::Texture, libcube::Texture>();
  installer.addType<j3d::Material, libcube::IGCMaterial>();
  installer.addType<j3d::Shape, libcube::IndexedPolygon>();
  installer.addType<j3d::Joint, libcube::IBoneDelegate>();

  ui::install();
}

} // namespace riistudio::j3d
