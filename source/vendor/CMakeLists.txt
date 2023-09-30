project(vendor)

# Necessary for LLVM
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Clang/Clang-Cl/AppleClang
if (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
  add_compile_options("-Wno-error")
endif()


include_directories(${PROJECT_SOURCE_DIR}/../)
include_directories(${PROJECT_SOURCE_DIR}/../vendor)
include_directories(${PROJECT_SOURCE_DIR}/../minizip/include)
include_directories(${PROJECT_SOURCE_DIR}/../plate/include)
include_directories(${PROJECT_SOURCE_DIR}/../plate/vendor)

file(GLOB_RECURSE SRC_FILES
	${PROJECT_SOURCE_DIR}/*.cxx
	${PROJECT_SOURCE_DIR}/*.cpp
	${PROJECT_SOURCE_DIR}/*.cc
	${PROJECT_SOURCE_DIR}/*.c
)
file(GLOB_RECURSE HDR_FILES
	${PROJECT_SOURCE_DIR}/*.hxx
	${PROJECT_SOURCE_DIR}/*.hpp
	${PROJECT_SOURCE_DIR}/*.h
)

list(FILTER SRC_FILES EXCLUDE REGEX ".*pybind11.*$")

add_library(vendor STATIC
	${SRC_FILES}
	${HDR_FILES}

  "ImGuizmo.cpp"
)

if (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang" OR ${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
  target_compile_options(vendor PRIVATE "-Wno-error")
endif()

target_link_libraries(vendor core)
