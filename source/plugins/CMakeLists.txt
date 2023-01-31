project(plugins)

include_directories(${PROJECT_SOURCE_DIR}/../)
include_directories(${PROJECT_SOURCE_DIR}/../vendor)
include_directories(${PROJECT_SOURCE_DIR}/../plate/include)
include_directories(${PROJECT_SOURCE_DIR}/../plate/vendor)


add_library(plugins STATIC
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
	
	"j3d/j3d_Installer.cpp"
	"j3d/j3d_Material.cpp"
	"j3d/Joint.hpp"
	"j3d/Material.hpp"
	"j3d/Model.hpp"
	"j3d/Node.h"
	
	"j3d/Scene.hpp"
	"j3d/Shape.cpp"
	"j3d/Shape.hpp"
	"j3d/Texture.hpp"
	"j3d/ui/MaterialView.cpp"
	"j3d/VertexBuffer.hpp"
  "mk/BFG/io/BFG.hpp"
  "mk/BFG/io/BFG.cpp"
  "mk/BFG/Fog.hpp"
  "mk/BFG/FogEntry.hpp"
  "mk/BFG/FogView.cpp"
  "mk/BFG/Node.h"
  "rhst/RHSTImporter.cpp" "g3d/ui/G3dSrtView.cpp"
  "g3d/G3dUi.cpp"  "g3d/G3dIo.hpp"
  "j3d/J3dIo.hpp"
  
  "mk/LTEX.cpp" "mk/LIGHT.cpp"
  "api.hpp"
  "api.cpp"
  "SceneImpl.cpp"
 "assimp/AssimpUI.cpp")

# CMake w/ ARM MacOS GCC does not work with PCH. Passes "-Xarch_arm" which is not valid on GCC.
if (NOT (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND ${CMAKE_SYSTEM_NAME} MATCHES "Darwin"))
  target_precompile_headers(plugins
  PUBLIC
    ../core/common.h
  )
endif()
target_link_libraries(plugins PUBLIC core PRIVATE range-v3)
