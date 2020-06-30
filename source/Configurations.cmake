# Adapted from
# https://stackoverflow.com/questions/31546278/where-to-set-cmake-configuration-types-in-a-project-with-subprojects
# {
if(NOT SET_UP_CONFIGURATIONS_DONE)
    set(SET_UP_CONFIGURATIONS_DONE TRUE)

    # No reason to set CMAKE_CONFIGURATION_TYPES if it's not a multiconfig generator
    # Also no reason mess with CMAKE_BUILD_TYPE if it's a multiconfig generator.
    get_property(isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if(isMultiConfig)
        set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo;MinSizeRel;Dist" CACHE STRING "" FORCE) 
    else()
        if(NOT CMAKE_BUILD_TYPE)
            message("Defaulting to release build.")
            set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
        endif()
        set_property(CACHE CMAKE_BUILD_TYPE PROPERTY HELPSTRING "Choose the type of build")
        # set the valid options for cmake-gui drop-down list
        set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug;Release;Dist")
    endif()
    # now set up the Dist configuration
    set(CMAKE_C_FLAGS_DIST ${CMAKE_C_FLAGS_RELEASE})
    set(CMAKE_CXX_FLAGS_DIST ${CMAKE_CXX_FLAGS_RELEASE})
    set(CMAKE_EXE_LINKER_FLAGS_DIST ${CMAKE_EXE_LINKER_FLAGS_RELEASE})
endif()
# }

if(CMAKE_CONFIGURATION_TYPES)
    string(TOLOWER "${CMAKE_CONFIGURATION_TYPES}" CMAKE_CONFIGURATION_TYPESURATION_TYPES_LOWER)
else()
    string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_CONFIGURATION_TYPES_LOWER)
endif()

foreach(config ${CMAKE_CONFIGURATION_TYPES_LOWER})
    if (${config} MATCHES "debug")
        add_definitions(-DDEBUG -DBUILD_DEBUG)
    elseif(${config} MATCHES "RelWithDebInfo")
        add_definitions(-DNDEBUG -DBUILD_RELEASE)
    else() # Release/MinSizeRel/Dist
        add_definitions(-DNDEBUG -DBUILD_DIST)
    endif()
endforeach()
