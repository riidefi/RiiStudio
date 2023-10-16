cmake_minimum_required(VERSION 3.8)

set(WARNINGS_AS_ERRORS_FOR_IMGUI_GRADIENT OFF CACHE BOOL "ON iff you want to treat warnings as errors")


include_directories(${PROJECT_SOURCE_DIR}/)
include_directories(${PROJECT_SOURCE_DIR}/vendor)
include_directories(${PROJECT_SOURCE_DIR}/plate/include)
include_directories(${PROJECT_SOURCE_DIR}/plate/vendor)


add_library(imgui_gradient)
add_library(imgui_gradient::imgui_gradient ALIAS imgui_gradient)
target_compile_features(imgui_gradient PUBLIC cxx_std_11)

# ---Add source files---
if(WARNINGS_AS_ERRORS_FOR_IMGUI_GRADIENT)
    target_include_directories(imgui_gradient PUBLIC include)
else()
    target_include_directories(imgui_gradient SYSTEM PUBLIC include)
endif()

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS src/*.cpp)
target_sources(imgui_gradient PRIVATE ${SRC_FILES})

# Set warning level
if(MSVC)
    target_compile_options(imgui_gradient PRIVATE /W4)
else()
    target_compile_options(imgui_gradient PRIVATE -Wall -Wextra -Wpedantic -pedantic-errors -Wconversion -Wsign-conversion)
endif()

# Maybe enable warnings as errors
if(WARNINGS_AS_ERRORS_FOR_IMGUI_GRADIENT)
    if(MSVC)
        target_compile_options(imgui_gradient PRIVATE /WX)
    else()
        target_compile_options(imgui_gradient PRIVATE -Werror)
    endif()
endif()
