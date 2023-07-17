project(core)

include_directories(${PROJECT_SOURCE_DIR}/../)
include_directories(${PROJECT_SOURCE_DIR}/../vendor)
include_directories(${PROJECT_SOURCE_DIR}/../plate/include)
include_directories(${PROJECT_SOURCE_DIR}/../plate/vendor)

add_library(core STATIC
  "common.h"
  "3d/Node.h"
  "util/timestamp.cpp"
   "util/oishii.hpp")

target_link_libraries(core PUBLIC fmt)
