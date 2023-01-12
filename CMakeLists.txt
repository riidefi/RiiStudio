cmake_minimum_required(VERSION 3.10)

project(RiiStudio VERSION 1.0)

set(CPM_DOWNLOAD_VERSION 0.36.0) 
set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")

if(NOT (EXISTS ${CPM_DOWNLOAD_LOCATION}))
    message(STATUS "Downloading CPM.cmake")
    file(DOWNLOAD https://github.com/TheLartians/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake ${CPM_DOWNLOAD_LOCATION})
endif()

include(${CPM_DOWNLOAD_LOCATION})

# Require C++20
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED False)

if (WIN32)
  # Windows + GLFW

  # Windows.h annoyance
  add_compile_definitions(NOMINMAX)

  add_compile_definitions(RII_PLATFORM_WINDOWS)
  add_compile_definitions(RII_BACKEND_GLFW)

  # Enable SSE3 if building with clang-cl
  if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang" AND "x${CMAKE_CXX_SIMULATE_ID}" STREQUAL "xMSVC")
    add_compile_options("-mssse3")
    if (FALSE)
      add_compile_options("-march=native")
    endif()
  endif()
elseif(APPLE)
  add_compile_definitions(RII_PLATFORM_MACOSX)
  add_compile_definitions(RII_BACKEND_GLFW)
  add_compile_definitions(LLVM_ON_UNIX)
  include_directories(/usr/local/include)
  link_directories(/usr/local/lib)
  add_compile_options(-DLLVM_DISABLE_ABI_BREAKING_CHECKS_ENFORCING=1)
elseif(UNIX)
  add_compile_definitions(RII_PLATFORM_LINUX)
  add_compile_definitions(RII_BACKEND_GLFW)
	add_compile_definitions(__linux__)
  add_compile_definitions(LLVM_ON_UNIX)
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
endif()

# Clang/Clang-Cl
# Match AppleClang
if (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
  add_compile_options("-Wno-inconsistent-missing-override")
  # LLVM
  add_compile_options("-Wno-ambiguous-reversed-operator")
  # GLM
  add_compile_options("-Wno-deprecated-volatile")
  # Assimp
  add_compile_options("-Wno-pragma-pack")

  if ("x${CMAKE_CXX_SIMULATE_ID}" STREQUAL "xMSVC")
    add_compile_options("/MT")
    string( REPLACE "/EHsc" "" CMAKE_CXX_FLAGS_INIT ${CMAKE_CXX_FLAGS_INIT})
    string( REPLACE "/EHsc" "" CMAKE_C_FLAGS_INIT ${CMAKE_C_FLAGS_INIT} )
    string( REPLACE "/EHs" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS_INIT} )
    string( REPLACE "/EHs" "" CMAKE_C_FLAGS_INIT ${CMAKE_C_FLAGS_INIT} )
    string( REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG} )
  endif()
  if (ASAN)
    add_compile_options("-fsanitize=address")
    set(ENV{ASAN_OPTIONS} "detect_leaks=1")
    add_compile_definitions(BUILD_ASAN)
  endif()
endif()
if (${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
  add_compile_options("-Wno-volatile")
  add_compile_options("-Wno-multichar")
endif()

if (MSVC)
  # LLVM headers
  add_compile_options(/wd4244 /wd4267 /wd4146 /wd4624 /wd4996)
  # Inherits via dominance (generated code)
  add_compile_options(/wd4250)
  add_link_options(-debug)
  # Modules support
  if (NOT ${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/experimental:module>)
  endif()
  # Standard modules are compiled with this
  string( REPLACE "/std:c++20" "/std:c++latest" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} )
  set(CMAKE_EXE_LINKER_FLAGS_DIST CMAKE_EXE_LINKER_FLAGS)
endif()

if (ASAN)
  add_compile_definitions(_DISABLE_VECTOR_ANNOTATION=1 _DISABLE_STRING_ANNOTATION=1)
endif()

add_compile_definitions(FMT_EXCEPTIONS=0)
CPMAddPackage("gh:fmtlib/fmt#9.1.0")
CPMAddPackage("gh:zeux/meshoptimizer#v0.18")

set(DRACO_BACKWARDS_COMPATIBILITY OFF)
set(DRACO_POINT_CLOUD_COMPRESSION OFF)
CPMAddPackage("gh:google/draco#1.5.5")

add_subdirectory(source)
