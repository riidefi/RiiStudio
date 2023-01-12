project(elzip)

add_subdirectory(extlibs/minizip)

file(GLOB LZIP_HEADERS include/elzip/*.hpp)

set(ELZIP_CPP src/elzip.cpp)

file(GLOB LZIP_SOURCES 
    ${ELZIP_CPP}
    src/unzipper.cpp
    src/zipper.cpp
)

add_library(elzip STATIC ${LZIP_HEADERS} ${LZIP_SOURCES})
target_link_libraries(elzip minizip)

target_include_directories(elzip
    PUBLIC 
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/elzip>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
