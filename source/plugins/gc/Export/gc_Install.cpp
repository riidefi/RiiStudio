#include <core/common.h>
#include <core/kpi/Node.hpp>

#include "Bone.hpp"
#include "IndexedPolygon.hpp"
#include "Material.hpp"
#include "Texture.hpp"

#include <core/kpi/RichNameManager.hpp>
#include <vendor/fa5/IconsFontAwesome5.h>

namespace libcube {

namespace UI {
void installDisplaySurface();
}

void Install() {
	assert(kpi::ApplicationPlugins::getInstance());
	kpi::ApplicationPlugins& installer = *kpi::ApplicationPlugins::getInstance();

	installer.registerParent<libcube::IBoneDelegate, riistudio::lib3d::Bone>()
		.registerParent<libcube::IGCMaterial, riistudio::lib3d::Material>()
		.registerParent<libcube::Texture, riistudio::lib3d::Texture>()
		.registerParent<libcube::IndexedPolygon, riistudio::lib3d::Polygon>();

	kpi::RichNameManager& rich = kpi::RichNameManager::getInstance();
	rich.addRichName<riistudio::lib3d::Bone>(ICON_FA_BONE, "Bone")
		.addRichName<riistudio::lib3d::Material>(ICON_FA_PAINT_BRUSH, "Material")
		.addRichName<riistudio::lib3d::Texture>(ICON_FA_IMAGE, "Texture", ICON_FA_IMAGES)
		.addRichName<riistudio::lib3d::Polygon>(ICON_FA_DRAW_POLYGON, "Polygon")
		.addRichName<riistudio::lib3d::Model>(ICON_FA_CUBE, "Model", ICON_FA_CUBES)
		.addRichName<riistudio::lib3d::Scene>(ICON_FA_SHAPES, "Scene");



	UI::installDisplaySurface();
}

} // namespace libcube
