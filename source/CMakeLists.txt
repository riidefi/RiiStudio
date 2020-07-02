# Require C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (WIN32)
  # Windows + GLFW

  # Windows.h annoyance
  add_compile_definitions(NOMINMAX)

  add_compile_definitions(RII_PLATFORM_WINDOWS)
  add_compile_definitions(RII_BACKEND_GLFW)

  # Enable SSE3 if building with clang-cl
  if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang" AND "x${CMAKE_CXX_SIMULATE_ID}" STREQUAL "xMSVC")
    add_compile_options("-mssse3")
  endif()
else()
  # Assume emscripten + GLES2/SDL (linux-like)
  
	add_compile_definitions(RII_PLATFORM_EMSCRIPTEN)
	add_compile_definitions(IMGUI_IMPL_OPENGL_ES2)
	add_compile_definitions(RII_BACKEND_SDL)
	add_compile_definitions(__linux__)

  # TODO: Set here, not toolchain
  # set(CMAKE_CXX_FLAGS "-s USE_SDL=2 -s ASSERTIONS=2")
  #add_compile_options("-s ALLOW_MEMORY_GROWTH=1")
  #add_compile_options("-s NO_EXIT_RUNTIME=0")
  add_compile_options("-Wno-inconsistent-missing-override")
  add_compile_options("-Wno-deprecated-volatile")
endif()

if (MSVC)
  # LLVM headers
  add_compile_options(/wd4244 /wd4267 /wd4146 /wd4624 /wd4996)
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/Configurations.cmake)

add_subdirectory(core)
add_subdirectory(plugins)
add_subdirectory(frontend)

# My libraries
add_subdirectory(oishii)
add_subdirectory(plate)

# Third-party libraries
add_subdirectory(vendor)