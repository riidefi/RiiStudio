#include "Installer.hpp"

#include "Scene.hpp"
#include "io/DMD.hpp"

namespace riistudio::pik {

void Install(kpi::ApplicationPlugins& installer) {
  InstallDMD(installer);

  installer.addType<pik::PikminModel, lib3d::Model>();
  installer.addType<pik::PikminCollection, lib3d::Scene>();
}

} // namespace riistudio::pik