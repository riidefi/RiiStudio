cmake_minimum_required(VERSION 3.10)

project(plate VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(PLATE_SOURCES
  source/Viewport.cpp
)

# <GLFW/glfw3native.h> vs <glfw/glfw3native.HDR_FILES> in imgui_impl_glfw.cpp
if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
  add_compile_options("-Wno-error")
endif()

if (WIN32 OR UNIX) # Windows + GLFW
  # Windows.h annoyance
  add_compile_definitions(NOMINMAX)

  add_compile_definitions(RII_PLATFORM_WINDOWS)
  add_compile_definitions(RII_BACKEND_GLFW)

  set(PLATE_SOURCES ${PLATE_SOURCES}
    source/glfw_windows/glfw_Platform.cpp
  )
else() # Assume emscripten + GLES2/SDL (linux-like)
	add_compile_definitions(RII_PLATFORM_EMSCRIPTEN)
	add_compile_definitions(IMGUI_IMPL_OPENGL_ES2)
	add_compile_definitions(RII_BACKEND_SDL)
	add_compile_definitions(__linux__)

  set(PLATE_SOURCES ${PLATE_SOURCES}
    source/sdl_emscripten/sdl_Platform.cpp
  )
endif()


include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/vendor)

file(GLOB_RECURSE VENDOR_SRC_FILES
	${PROJECT_SOURCE_DIR}/vendor/*.cpp
	${PROJECT_SOURCE_DIR}/vendor/*.c

  ${PROJECT_SOURCE_DIR}/vendor/imgui/misc/freetype/imgui_freetype.cpp
)
file(GLOB_RECURSE VENDOR_HDR_FILES
	${PROJECT_SOURCE_DIR}/vendor/*.hpp
	${PROJECT_SOURCE_DIR}/vendor/*.h
)

add_library(plate
  ${PLATE_SOURCES}
	${VENDOR_SRC_FILES}
	${VENDOR_HDR_FILES}
)
