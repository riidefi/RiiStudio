project(librii)

include_directories(${PROJECT_SOURCE_DIR}/../)
include_directories(${PROJECT_SOURCE_DIR}/../vendor)
include_directories(${PROJECT_SOURCE_DIR}/../plate/include)
include_directories(${PROJECT_SOURCE_DIR}/../plate/vendor)

add_library(librii STATIC
  "gl/Compiler.hpp"
  "gl/Compiler.cpp"
  "gl/EnumConverter.hpp"
  "gl/EnumConverter.cpp"

  "gx/Texture.cpp"
  "gx/validate/MaterialValidate.hpp"
  "gx/validate/MaterialValidate.cpp"
  "hx/PixMode.hpp"
  "gx/Polygon.hpp"

  "kmp/CourseMap.hpp"
  "kmp/CourseMap.cpp"
  "kmp/io/KMP.cpp"
  "kmp/io/KMP.hpp"

  "szs/SZS.hpp"
  "szs/SZS.cpp"

  "nitro/types.hpp"

  "image/CmprEncoder.cpp"
  "image/CmprEncoder.hpp"
  "image/ImagePlatform.cpp"
  "image/ImagePlatform.hpp"
  "image/TextureExport.cpp"
  "image/TextureExport.hpp"
  "image/CheckerBoard.hpp"

  "gpu/DLBuilder.hpp"
  "gpu/DLInterpreter.cpp"
  "gpu/DLInterpreter.hpp"
  "gpu/DLPixShader.cpp"
  "gpu/DLPixShader.hpp"
  "gpu/GPUAddressSpace.hpp"
  "gpu/GPUCommand.hpp"
  "gpu/GPUMaterial.cpp"
  "gpu/GPUMaterial.hpp"
  "gpu/DLMesh.hpp"
  "gpu/DLMesh.cpp"
 
  "mtx/TexMtx.cpp"
  "mtx/TexMtx.hpp"

  "hx/CullMode.hpp"
  "hx/TexGenType.hpp"
  "hx/TextureFilter.hpp"
  "hx/KonstSel.hpp"

  "gfx/MegaState.hpp"
  "gfx/SceneState.cpp"
  
  "glhelper/ShaderCache.cpp"
  "glhelper/ShaderProgram.cpp"
  "glhelper/UBOBuilder.cpp"
  "glhelper/VBOBuilder.cpp"
  "glhelper/Util.cpp"  
  "glhelper/Primitives.cpp"

  "rhst/RHST.hpp"
  "rhst/RHST.cpp"

  "math/aabb.hpp"
  "math/srt3.hpp"

  "kcol/SerializationProfile.hpp"
  "kcol/SerializationProfile.cpp"

  "g3d/data/TextureData.hpp"
  "g3d/data/BoneData.hpp"
  "g3d/data/MaterialData.hpp"
  "g3d/data/PolygonData.hpp"
  "g3d/data/ModelData.hpp"

  "j3d/data/JointData.hpp"
  "j3d/data/TextureData.hpp"
  "j3d/data/MaterialData.hpp"
  "j3d/data/ShapeData.hpp"
  "g3d/io/TextureIO.hpp"
  "g3d/io/TextureIO.cpp"
  "g3d/data/AnimData.hpp"
  "g3d/io/AnimIO.cpp"
  "g3d/io/DictIO.hpp"
  "g3d/io/CommonIO.hpp"
  "g3d/io/AnimIO.hpp"
  "g3d/io/TevIO.hpp"
  "g3d/io/TevIO.cpp"
  "g3d/io/NameTableIO.cpp"
  "g3d/io/DictWriteIO.hpp"
  "g3d/io/DictWriteIO.cpp"
  "u8/U8.cpp" "u8/U8.hpp"
  "gfx/PixelOcclusion.hpp"
  "gfx/TextureObj.hpp" "gfx/TextureObj.cpp"
  "gfx/SceneNode.hpp" "gfx/SceneNode.cpp"
  "glhelper/GlTexture.hpp" "glhelper/GlTexture.cpp"
  "kcol/Model.hpp" "kcol/Model.cpp"
  "g3d/gfx/G3dGfx.hpp" "g3d/gfx/G3dGfx.cpp"
  "g3d/io/MatIO.cpp" "g3d/io/MatIO.hpp"
  "g3d/io/BoneIO.cpp"
  "g3d/data/VertexData.hpp"
  "g3d/io/ModelIO.hpp"
  "g3d/io/ModelIO.cpp"
  "g3d/io/ArchiveIO.hpp"
  "g3d/io/ArchiveIO.cpp"
  "crate/g3d_crate.cpp"
  "egg/Blight.cpp"
  "g3d/io/AnimTexPatIO.cpp"
  "g3d/io/AnimClrIO.cpp"
  "g3d/io/AnimVisIO.cpp"
  "egg/LTEX.cpp"
  "g3d/io/WiiTrig.cpp"

  "j3d/io/BMD.cpp"
	"j3d/io/OutputCtx.hpp"
	"j3d/io/SceneGraph.cpp"
	"j3d/io/SceneGraph.hpp"
	"j3d/io/Sections/EVP1DRW1.cpp"
	"j3d/io/Sections/INF1.cpp"
	"j3d/io/Sections/JNT1.cpp"
	"j3d/io/Sections/MAT3.cpp"
	"j3d/io/Sections/MDL3.cpp"
	"j3d/io/Sections/SHP1.cpp"
	"j3d/io/Sections/TEX1.cpp"
	"j3d/io/Sections/VTX1.cpp"
	"j3d/io/Sections.hpp"
	"j3d/io/Sections/MaterialData.cpp"
 "rhst/RHSTOptimizer.cpp" "rhst/MeshUtils.cpp"  "g3d/io/PolygonIO.cpp"


	"assimp2rhst/Assimp.cpp"
  "assimp2rhst/Importer.cpp"
  "assimp2rhst/Importer.hpp"
  "assimp2rhst/Logger.hpp"
  "assimp2rhst/Utility.hpp"
  "assimp2rhst/Assimp.hpp"
  "assimp2rhst/SupportedFiles.hpp"
  "assimp2rhst/SupportedFiles.cpp"
  "assimp2rhst/InclusionMask.hpp"
  "assimp2rhst/InclusionMask.cpp"
  "assimp2rhst/LogScope.hpp"
  "assimp2rhst/LogScope.cpp"
 "egg/BDOF.cpp"

 "sp/DebugClient.cpp"
 "sp/detail/FFI.h" "egg/PBLM.cpp" "egg/BFG.cpp"

"tev/TevSolver.cpp"
 "assimp/LRAssimp.cpp"
"assimp/LRAssimpJSON.cpp" "objflow/ObjFlow.cpp" "lettuce/LettuceLEX.cpp" "j3d/BinaryBTK.cpp" "g3d/io/AnimChrIO.cpp")

# CMake w/ ARM MacOS GCC does not work with PCH. Passes "-Xarch_arm" which is not valid on GCC.
if (NOT (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND ${CMAKE_SYSTEM_NAME} MATCHES "Darwin"))
  target_precompile_headers(librii
  PUBLIC
    ../core/common.h
  )
endif()

if (UNIX)
  set(LINUX_LIBS curl)
endif (UNIX)

target_link_libraries(librii PUBLIC core LibBadUIFramework rsmeshopt  PRIVATE fort range-v3 json_struct vendor oishii gctex ${LINUX_LIBS})

target_include_directories(librii
  PRIVATE
    $<BUILD_INTERFACE:${draco_SOURCE_DIR}>/src/
)
