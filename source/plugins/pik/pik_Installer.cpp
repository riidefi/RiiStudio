#include "Scene.hpp"
#include "io/DMD.hpp"

namespace riistudio::pik {

kpi::DecentralizedInstaller Installer([](kpi::ApplicationPlugins& installer) {
  installer.addType<pik::PikminModel, lib3d::Model>();
  installer.addType<pik::PikminCollection, lib3d::Scene>();
});

} // namespace riistudio::pik
