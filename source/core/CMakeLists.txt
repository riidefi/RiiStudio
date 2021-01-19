project(core)

include_directories(${PROJECT_SOURCE_DIR}/../)
include_directories(${PROJECT_SOURCE_DIR}/../vendor)
include_directories(${PROJECT_SOURCE_DIR}/../plate/include)
include_directories(${PROJECT_SOURCE_DIR}/../plate/vendor)

add_library(core
  "api.cpp"
  "common.h"
  "3d/Node.h"
  "3d/Scene.cpp"
  "3d/renderer/SceneState.cpp"
  "3d/renderer/SceneTree.cpp"
  "3d/renderer/ShaderCache.cpp"
  "3d/renderer/ShaderProgram.cpp"
  "3d/renderer/UBOBuilder.cpp"
  "3d/renderer/VBOBuilder.cpp"
  "kpi/ActionMenu.cpp"
  "kpi/Memento.cpp"
  "kpi/Plugins.cpp"
  "kpi/PropertyView.cpp"
  "kpi/Reflection.cpp"
  "kpi/RichNameManager.cpp"
  "util/timestamp.cpp"
)
