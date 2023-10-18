cmake_minimum_required(VERSION 3.26)

project(rsmeshopt_example)

# Require C++23 (std::expected)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(download_szs)

add_executable(your_cpp_bin main.cpp)
target_link_libraries(your_cpp_bin PRIVATE szs)
add_custom_command(TARGET your_cpp_bin POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:your_cpp_bin> $<TARGET_FILE_DIR:your_cpp_bin> 
  COMMAND_EXPAND_LISTS
)
