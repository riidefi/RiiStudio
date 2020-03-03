#include <core/kpi/Node.hpp>
#include <core/common.h>

#include "model.hpp"
#include "collection.hpp"

#include "io/BRRES.hpp"

namespace riistudio::g3d  {

void Install()
{
    assert(kpi::ApplicationPlugins::getInstance());
    kpi::ApplicationPlugins& installer = *kpi::ApplicationPlugins::getInstance();

	installer.addType<g3d::Bone>()
		.registerParent<g3d::Bone, libcube::IBoneDelegate>();

    installer.addType<g3d::Texture>()
             .registerParent<g3d::Texture, libcube::Texture>()
             .addType<g3d::Material>()
             .registerParent<g3d::Material, libcube::IGCMaterial>()
             .addType<g3d::G3DModel>()
             .addType<g3d::G3DCollection>()
             .registerParent<g3d::G3DCollection, lib3d::Scene>();

	InstallBRRES(installer);
}

} // namespace riistudio::g3d 
