cmake_minimum_required(VERSION 3.22)

# Must be set before project() to take effect
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

project(SonicLab VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(SONIC_LAB_BUILD_SNAPSHOT_TOOL "Also build a <Plugin>Snapshot app per plugin that renders its editor to a PNG" OFF)

# JUCE is a git submodule pinned to a release tag
add_subdirectory(external/JUCE)

include(cmake/TriadPlugin.cmake)

# Shared code and branding used by every plugin
juce_add_binary_data(TriadAssets
    HEADER_NAME TriadAssets.h
    NAMESPACE TriadAssets
    SOURCES
        shared/assets/triad_logo.png)

juce_add_module(shared/triad_shared)
target_link_libraries(triad_shared INTERFACE TriadAssets)

# Every folder under plugins/ that has a CMakeLists.txt is a plugin
file(GLOB plugin_dirs LIST_DIRECTORIES true "${CMAKE_CURRENT_SOURCE_DIR}/plugins/*")

foreach(plugin_dir IN LISTS plugin_dirs)
    if(EXISTS "${plugin_dir}/CMakeLists.txt")
        add_subdirectory("${plugin_dir}")
    endif()
endforeach()
