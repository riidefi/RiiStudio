project(oishii)

include_directories(${PROJECT_SOURCE_DIR}/../)
include_directories(${PROJECT_SOURCE_DIR}/../vendor)
include_directories(${PROJECT_SOURCE_DIR}/../plate/include)
include_directories(${PROJECT_SOURCE_DIR}/../plate/vendor)

file(GLOB_RECURSE SRC_FILES
	${PROJECT_SOURCE_DIR}/*.cxx
)
file(GLOB_RECURSE HDR_FILES
	${PROJECT_SOURCE_DIR}/*.hxx
	${PROJECT_SOURCE_DIR}/*.hpp
	${PROJECT_SOURCE_DIR}/*.h
)

add_library(oishii STATIC
	${SRC_FILES}
	${HDR_FILES}
 "AbstractStream.cxx")
target_link_libraries(oishii core)
