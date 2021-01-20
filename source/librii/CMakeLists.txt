project(librii)

include_directories(${PROJECT_SOURCE_DIR}/../)
include_directories(${PROJECT_SOURCE_DIR}/../vendor)
include_directories(${PROJECT_SOURCE_DIR}/../plate/include)
include_directories(${PROJECT_SOURCE_DIR}/../plate/vendor)


add_library(librii
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
  )
