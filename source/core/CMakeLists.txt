project(core)

include_directories(${PROJECT_SOURCE_DIR}/../)
include_directories(${PROJECT_SOURCE_DIR}/../vendor)
include_directories(${PROJECT_SOURCE_DIR}/../plate/include)
include_directories(${PROJECT_SOURCE_DIR}/../plate/vendor)

add_library(core STATIC
  "common.h"
  "3d/Node.h"
  
  "kpi/ActionMenu.cpp"
  "kpi/Memento.cpp"
  "kpi/Plugins.cpp"
  "kpi/PropertyView.cpp"
  "kpi/Reflection.cpp"
  "kpi/RichNameManager.cpp"
  "util/timestamp.cpp"
   "util/oishii.hpp")
