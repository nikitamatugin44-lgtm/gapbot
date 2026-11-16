
cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(ExtendedPrecision VERSION 1.2.0)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    src/*.cpp
    src/*.hpp
)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR
        "Unable to find Geode SDK! "
        "Set GEODE_SDK environment variable."
    )
endif()

message(STATUS "Found Geode: $ENV{GEODE_SDK}")

add_subdirectory(
    $ENV{GEODE_SDK}
    ${CMAKE_CURRENT_BINARY_DIR}/geode
)

setup_geode_mod(${PROJECT_NAME})
