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
  # LLVM
  add_compile_options(-Wno-deprecated-declarations)
  # Draco
  add_compile_options(-Wno-implicit-const-int-float-conversion)
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
  # Lots of C++14 code
  add_compile_definitions(_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING=1)

  if ("x${CMAKE_CXX_SIMULATE_ID}" STREQUAL "xMSVC")
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

# Force static CRT runtime.    
#
# MSVC:
if (WIN32)
  string( REPLACE "/MDd" "MTd" CMAKE_CXX_FLAGS_INIT ${CMAKE_CXX_FLAGS_INIT} )
  string( REPLACE "/MD" "MT" CMAKE_CXX_FLAGS_INIT ${CMAKE_CXX_FLAGS_INIT} )
endif()

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
  add_compile_options("-U _DLL")
  if (WIN32)
    # Include a PDB
    add_link_options("-g")
  endif()
endif()


add_compile_definitions(FMT_EXCEPTIONS=0)
CPMAddPackage("gh:fmtlib/fmt#10.0.0")
CPMAddPackage("gh:zeux/meshoptimizer#v0.18")

# Doesn't work OOB -- requires exceptions
if (FALSE)
  CPMAddPackage(
    NAME cppcoro
    GIT_REPOSITORY https://github.com/lewissbaker/cppcoro
    GIT_TAG master
    DOWNLOAD_ONLY YES
  )
  if (cppcoro_ADDED)
    FILE(GLOB cppcoro_sources ${cppcoro_SOURCE_DIR}/lib/*.cpp)
    add_library(cppcoro STATIC ${cppcoro_sources})
    target_include_directories(cppcoro
    PUBLIC
      $<BUILD_INTERFACE:${cppcoro_SOURCE_DIR}/include>
    )
  endif()
endif()

# Doesn't work OOB -- they explicitly block Clang
if (FALSE)
  set(LIBCORO_BUILD_TESTS OFF)
  set(LIBCORO_CODE_COVERAGE OFF)
  set(LIBCORO_BUILD_EXAMPLES OFF)
  CPMAddPackage("gh:jbaldwin/libcoro#main")
  target_include_directories(libcoro
  PUBLIC
    $<BUILD_INTERFACE:${libcoro_SOURCE_DIR}/inc>
  )
endif()

add_definitions(-DINCLUDE_RUST_HEADERS)

if (APPLE)
  execute_process(COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE ARCHITECTURE)
  if (${ARCHITECTURE} STREQUAL "arm64")
    set(Rust_CARGO_TARGET "aarch64-apple-darwin")
  endif()
endif()

CPMAddPackage("gh:riidefi/corrosion#master")
if(NOT UNIX)
  find_package(Corrosion REQUIRED)
endif()

corrosion_import_crate(MANIFEST_PATH source/rust/Cargo.toml CRATE_TYPES cdylib)
if (NOT WIN32)
  corrosion_import_crate(MANIFEST_PATH source/llvm/Cargo.toml CRATE_TYPES cdylib)
endif()
corrosion_import_crate(MANIFEST_PATH source/gctex/Cargo.toml CRATE_TYPES cdylib)
corrosion_import_crate(MANIFEST_PATH source/avir-rs/Cargo.toml CRATE_TYPES cdylib)
corrosion_import_crate(MANIFEST_PATH source/c-discord-rich-presence/Cargo.toml CRATE_TYPES staticlib)
corrosion_import_crate(MANIFEST_PATH source/dolphin-memory-engine-rs/Cargo.toml CRATE_TYPES cdylib)
corrosion_import_crate(MANIFEST_PATH source/c-wbz/Cargo.toml CRATE_TYPES staticlib)
corrosion_import_crate(MANIFEST_PATH source/cli/rszst_arg_parser/Cargo.toml CRATE_TYPES staticlib)

add_subdirectory(source)
