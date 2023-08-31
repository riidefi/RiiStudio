project(rsmeshopt)

include_directories(${PROJECT_SOURCE_DIR}/../)
include_directories(${PROJECT_SOURCE_DIR}/../vendor)
include_directories(${PROJECT_SOURCE_DIR}/../plate/include)
include_directories(${PROJECT_SOURCE_DIR}/../plate/vendor)

include_directories(${PROJECT_SOURCE_DIR}/)

file(GLOB_RECURSE SRC_FILES
	${PROJECT_SOURCE_DIR}/*.cxx
	${PROJECT_SOURCE_DIR}/*.cpp
	${PROJECT_SOURCE_DIR}/*.c
)

add_library(rsmeshopt STATIC
  ${SRC_FILES}
  "HaroohieTriStripifier.cpp"
  "RsMeshOpt.cpp"
  "TriangleFanSplitter.cpp"

  "draco/attributes/point_attribute.cc"
  "draco/attributes/geometry_attribute.cc"
  "draco/core/bounding_box.cc"
  "draco/core/data_buffer.cc"
  "draco/core/draco_types.cc"
  "draco/mesh/corner_table.cc"
  "draco/mesh/mesh.cc"
  "draco/mesh/mesh_misc_functions.cc"
  "draco/mesh/mesh_stripifier.cc"
  "draco/point_cloud/point_cloud.cc"

  "meshoptimizer/stripifier.cpp"
)
target_link_libraries(rsmeshopt PUBLIC core riistudio_rs vendor)

if (WIN32)
  if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    target_compile_options(rsmeshopt PRIVATE "-O2")
  else()
    target_compile_options(rsmeshopt PRIVATE "/Ox")
  endif()
endif()
