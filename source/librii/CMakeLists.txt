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
  "g3d/io/AnimIO.hpp" "g3d/io/TevIO.hpp" "g3d/io/TevIO.cpp" "g3d/io/NameTableIO.cpp" "g3d/io/DictWriteIO.hpp" "g3d/io/DictWriteIO.cpp" "u8/U8.cpp" "u8/U8.hpp" "image/DsTexture.hpp" "image/DsTexture.cpp" "gfx/PixelOcclusion.hpp" "gfx/TextureObj.hpp" "gfx/TextureObj.cpp" "gfx/SceneNode.hpp" "gfx/SceneNode.cpp" "glhelper/GlTexture.hpp" "glhelper/GlTexture.cpp" "kcol/Model.hpp" "kcol/Model.cpp" "g3d/gfx/G3dGfx.hpp" "g3d/gfx/G3dGfx.cpp" "g3d/io/MatIO.cpp" "g3d/io/MatIO.hpp" "g3d/io/BoneIO.cpp" "g3d/data/VertexData.hpp" "g3d/io/ModelIO.hpp" "g3d/io/ModelIO.cpp" "g3d/io/ArchiveIO.hpp"
  "g3d/io/ArchiveIO.cpp"
  
  "crate/g3d_crate.cpp"
  )
