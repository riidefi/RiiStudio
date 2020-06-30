project(core)

include_directories(${PROJECT_SOURCE_DIR}/../)
include_directories(${PROJECT_SOURCE_DIR}/../vendor)
include_directories(${PROJECT_SOURCE_DIR}/../plate/include)
include_directories(${PROJECT_SOURCE_DIR}/../plate/vendor)

file(GLOB_RECURSE SRC_FILES
	${PROJECT_SOURCE_DIR}/*.cpp
)
file(GLOB_RECURSE HDR_FILES
	${PROJECT_SOURCE_DIR}/*.hpp
	${PROJECT_SOURCE_DIR}/*.h
)

add_library(core
	${SRC_FILES}
	${HDR_FILES}
)
