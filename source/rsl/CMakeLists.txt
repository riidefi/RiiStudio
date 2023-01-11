project(rsl)

include_directories(${PROJECT_SOURCE_DIR}/../)
include_directories(${PROJECT_SOURCE_DIR}/../vendor)
include_directories(${PROJECT_SOURCE_DIR}/../plate/include)
include_directories(${PROJECT_SOURCE_DIR}/../plate/vendor)


add_library(rsl STATIC
  "FsDialog.cpp"
 "Defer.hpp" "DebugBreak.hpp" "Ranges.hpp" "Stb.cpp")
target_link_libraries(rsl core)
