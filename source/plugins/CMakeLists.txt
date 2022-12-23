project(plugins)

include_directories(${PROJECT_SOURCE_DIR}/../)
include_directories(${PROJECT_SOURCE_DIR}/../vendor)
include_directories(${PROJECT_SOURCE_DIR}/../plate/include)
include_directories(${PROJECT_SOURCE_DIR}/../plate/vendor)


add_library(plugins
	"g3d/bone.hpp"
	"g3d/collection.hpp"
	"g3d/g3d_install.cpp"
	"g3d/g3d_material.cpp"
	"g3d/g3d_polygon.cpp"
	"g3d/io/BRRES.cpp"
  
  "g3d/ui/G3dVertexColorView.cpp"
	"g3d/material.hpp"
	"g3d/model.hpp"
	"g3d/Node.h"
	"g3d/polygon.hpp"
	"g3d/texture.hpp"
	
	
	
	"gc/Export/Bone.hpp"
	"gc/Export/gc_Install.cpp"
	"gc/Export/IndexedPolygon.cpp"
	"gc/Export/IndexedPolygon.hpp"
	
	
	"gc/Export/Material.hpp"
	
	"gc/Export/Scene.hpp"
	"gc/Export/Texture.hpp"
	
  

	"gc/GX/Material.cpp"
	
	
	"gc/Node.h"
	
	"j3d/io/BMD.cpp"
	"j3d/io/OutputCtx.hpp"
	"j3d/io/SceneGraph.cpp"
	"j3d/io/SceneGraph.hpp"
	"j3d/io/Sections/EVP1DRW1.cpp"
	"j3d/io/Sections/INF1.cpp"
	"j3d/io/Sections/JNT1.cpp"
	"j3d/io/Sections/MAT3.cpp"
	"j3d/io/Sections/MaterialData.hpp"
	"j3d/io/Sections/MDL3.cpp"
	"j3d/io/Sections/SHP1.cpp"
	"j3d/io/Sections/TEX1.cpp"
	"j3d/io/Sections/VTX1.cpp"
	"j3d/io/Sections.hpp"
	"j3d/j3d_Installer.cpp"
	"j3d/j3d_Material.cpp"
	"j3d/Joint.hpp"
	"j3d/Material.hpp"
	"j3d/Model.hpp"
	"j3d/Node.h"
	"j3d/Scene.cpp"
	"j3d/Scene.hpp"
	"j3d/Shape.cpp"
	"j3d/Shape.hpp"
	"j3d/Texture.hpp"
	"j3d/ui/MaterialView.cpp"
	"j3d/VertexBuffer.hpp"
	"ass/Ass.cpp"
  "ass/AssImporter.cpp"
  "ass/AssImporter.hpp"
  "ass/AssLogger.hpp"
  "ass/Utility.hpp"
  "ass/AssMaterial.hpp"
  
  
  
  "mk/BFG/io/BFG.hpp"
  "mk/BFG/io/BFG.cpp"
  "mk/BFG/Fog.hpp"
  "mk/BFG/FogEntry.hpp"
  "mk/BFG/FogView.cpp"
  "mk/BFG/Node.h"
  
 "rhst/RHSTImporter.cpp" "g3d/ui/G3dSrtView.cpp" "g3d/G3dUi.cpp"  "g3d/G3dIo.hpp" "j3d/J3dIo.hpp" "ass/Ass.hpp" "ass/SupportedFiles.hpp" "ass/SupportedFiles.cpp" "ass/InclusionMask.hpp" "ass/InclusionMask.cpp" "ass/LogScope.hpp" "ass/LogScope.cpp" "ass/ImportTexture.hpp" "ass/ImportTexture.cpp")

 target_precompile_headers(plugins
  PUBLIC
    ../core/common.h
)
