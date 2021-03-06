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
    if (FALSE)
      add_compile_options("-march=native")
    endif()
  endif()
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
if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
  add_compile_options("-Wno-inconsistent-missing-override")
  # LLVM
  add_compile_options("-Wno-ambiguous-reversed-operator")
  # GLM
  add_compile_options("-Wno-deprecated-volatile")
  # Assimp
  add_compile_options("-Wno-pragma-pack")
  add_compile_options("-Werror")

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

if (MSVC)
  # LLVM headers
  add_compile_options(/wd4244 /wd4267 /wd4146 /wd4624 /wd4996)
  # Inherits via dominance (generated code)
  add_compile_options(/wd4250)
  add_link_options(-debug)
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/Configurations.cmake)

add_subdirectory(core)
add_subdirectory(plugins)
add_subdirectory(frontend)
add_subdirectory(tests)

# My libraries
add_subdirectory(oishii)
add_subdirectory(plate)

add_subdirectory(librii)

# Third-party libraries
add_subdirectory(vendor)
add_subdirectory(11zip)
