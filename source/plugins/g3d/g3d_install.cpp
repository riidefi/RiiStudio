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

	installer
		.addType<g3d::Bone, libcube::IBoneDelegate>()
		.addType<g3d::Texture, libcube::Texture>()
		.addType<g3d::Material, libcube::IGCMaterial>()
		.addType<g3d::G3DModel, lib3d::Model>()
		.addType<g3d::G3DCollection, lib3d::Scene>()
		.addType<g3d::PositionBuffer>()
		.addType<g3d::NormalBuffer>()
		.addType<g3d::ColorBuffer>()
		.addType<g3d::TextureCoordinateBuffer>()
		.addType<g3d::Polygon, libcube::IndexedPolygon>();

	InstallBRRES(installer);
}

} // namespace riistudio::g3d 
