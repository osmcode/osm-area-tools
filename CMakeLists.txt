#-----------------------------------------------------------------------------
#
#  CMake Config
#
#  OSM Area Tools
#
#-----------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.7 FATAL_ERROR)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

project(osmium-area VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#-----------------------------------------------------------------------------
#
#  Find external dependencies
#
#-----------------------------------------------------------------------------

find_package(Osmium 2.15.4 COMPONENTS io ogr geos)
include_directories(SYSTEM ${OSMIUM_INCLUDE_DIRS} include)


#-----------------------------------------------------------------------------
#
#  Compiler and Linker flags
#
#-----------------------------------------------------------------------------
set(CMAKE_CXX_FLAGS_DEV "-O3 -g -fno-omit-frame-pointer"
    CACHE STRING "Flags used by the compiler during developer builds."
    FORCE)

set(CMAKE_EXE_LINKER_FLAGS_DEV ""
    CACHE STRING "Flags used by the linker during developer builds."
    FORCE)
mark_as_advanced(
    CMAKE_CXX_FLAGS_DEV
    CMAKE_EXE_LINKER_FLAGS_DEV
)

set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g -DNDEBUG"
    CACHE STRING "Flags used by the compiler during RELWITHDEBINFO builds."
    FORCE)


#-----------------------------------------------------------------------------
#
#  Build Type
#
#-----------------------------------------------------------------------------
set(CMAKE_CONFIGURATION_TYPES "Debug Release RelWithDebInfo MinSizeRel Dev")

# In 'Dev' mode: compile with very strict warnings and turn them into errors.
if(CMAKE_BUILD_TYPE STREQUAL "Dev")
    if(NOT MSVC)
        add_definitions(-Werror -fno-omit-frame-pointer)
    endif()
    add_definitions(${OSMIUM_WARNING_OPTIONS})
endif()

# Force RelWithDebInfo build type if none was given
if(CMAKE_BUILD_TYPE)
    set(build_type ${CMAKE_BUILD_TYPE})
else()
    set(build_type "RelWithDebInfo")
endif()

set(CMAKE_BUILD_TYPE ${build_type}
    CACHE STRING
    "Choose the type of build, options are: ${CMAKE_CONFIGURATION_TYPES}."
    FORCE)


#-----------------------------------------------------------------------------
#
#  Optional "clang-tidy" target
#
#-----------------------------------------------------------------------------
message(STATUS "Looking for clang-tidy")
find_program(CLANG_TIDY NAMES clang-tidy clang-tidy-16 clang-tidy-15 clang-tidy-14 clang-tidy-13 clang-tidy-12 clang-tidy-11)

if(CLANG_TIDY)
    message(STATUS "Looking for clang-tidy - found ${CLANG_TIDY}")

    file(GLOB _all_code src/*.cpp)

    add_custom_target(clang-tidy
        ${CLANG_TIDY}
        -p ${CMAKE_BINARY_DIR}
        ${_all_code}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/src"
    )
else()
    message(STATUS "Looking for clang-tidy - not found")
    message(STATUS "  Build target 'clang-tidy' will not be available.")
endif()


#-----------------------------------------------------------------------------
#
#  Optional "cppcheck" target that checks C++ code
#
#-----------------------------------------------------------------------------
message(STATUS "Looking for cppcheck")
find_program(CPPCHECK cppcheck)

if(CPPCHECK)
    message(STATUS "Looking for cppcheck - found")
    set(CPPCHECK_OPTIONS --enable=all)

    # cpp doesn't find system includes for some reason, suppress that report
    set(CPPCHECK_OPTIONS ${CPPCHECK_OPTIONS} --suppress=missingIncludeSystem)

    file(GLOB ALL_CODE src/*.cpp)

    set(CPPCHECK_FILES ${ALL_CODE})

    add_custom_target(cppcheck
        ${CPPCHECK}
        --std=c++11 ${CPPCHECK_OPTIONS}
        ${CPPCHECK_FILES}
    )
else()
    message(STATUS "Looking for cppcheck - not found")
    message(STATUS "  Build target 'cppcheck' will not be available.")
endif(CPPCHECK)


#-----------------------------------------------------------------------------

add_definitions(${OSMIUM_WARNING_OPTIONS})

add_subdirectory(src)


#-----------------------------------------------------------------------------
