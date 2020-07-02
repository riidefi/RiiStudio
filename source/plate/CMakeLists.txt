project(plate)

# <GLFW/glfw3native.h> vs <glfw/glfw3native.HDR_FILES> in imgui_impl_glfw.cpp
if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
  add_compile_options("-Wno-error")
endif()

include_directories(${PROJECT_SOURCE_DIR}/../)
include_directories(${PROJECT_SOURCE_DIR}/../vendor)
include_directories(${PROJECT_SOURCE_DIR}/../plate/include)
include_directories(${PROJECT_SOURCE_DIR}/../plate/vendor)

file(GLOB_RECURSE SRC_FILES
	${PROJECT_SOURCE_DIR}/*.cpp
	${PROJECT_SOURCE_DIR}/*.c
)
file(GLOB_RECURSE HDR_FILES
	${PROJECT_SOURCE_DIR}/*.hpp
	${PROJECT_SOURCE_DIR}/*.h
)

add_library(plate
	${SRC_FILES}
	${HDR_FILES}
)
