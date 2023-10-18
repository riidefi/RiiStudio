cmake_minimum_required(VERSION 3.26)
project(gctex_example)

add_subdirectory(download_gctex)

add_executable(your_cpp_bin main.cpp)
target_link_libraries(your_cpp_bin PRIVATE gctex)
add_custom_command(TARGET your_cpp_bin POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:your_cpp_bin> $<TARGET_FILE_DIR:your_cpp_bin> 
  COMMAND_EXPAND_LISTS
)
