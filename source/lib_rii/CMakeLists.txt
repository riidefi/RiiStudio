project(lib_rii)

include_directories(${PROJECT_SOURCE_DIR}/../)
include_directories(${PROJECT_SOURCE_DIR}/../vendor)
include_directories(${PROJECT_SOURCE_DIR}/../plate/include)
include_directories(${PROJECT_SOURCE_DIR}/../plate/vendor)


add_library(lib_rii
  lib_rii.cpp
  "gl/Compiler.hpp"
  "gl/Compiler.cpp"
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

 "gx/validate/MaterialValidate.hpp" "gx/validate/MaterialValidate.cpp" "gx/Texture.cpp")
