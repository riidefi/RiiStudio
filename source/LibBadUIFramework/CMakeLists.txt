project(LibBadUIFramework)

include_directories(${PROJECT_SOURCE_DIR}/../)
include_directories(${PROJECT_SOURCE_DIR}/../vendor)
include_directories(${PROJECT_SOURCE_DIR}/../plate/include)
include_directories(${PROJECT_SOURCE_DIR}/../plate/vendor)


add_library(LibBadUIFramework STATIC
  "ActionMenu.cpp"
  
  
  
  
)
target_link_libraries(LibBadUIFramework core)
set(LibBadUIFramework_HDR
        "ActionMenu.hpp" "Memento.hpp" "Plugins.hpp" "PropertyView.hpp" "Reflection.hpp" "RichNameManager.hpp"
)
set_target_properties(LibBadUIFramework PROPERTIES PUBLIC_HEADER "${LibBadUIFramework_HDR}")
