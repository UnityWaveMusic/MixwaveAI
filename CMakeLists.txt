cmake_minimum_required(VERSION 3.22)

# Static MSVC runtime: the plugins do not need the VC++ redistributable on the
# user's machine. Must be set before any target is created.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(MixwaveAI VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# JUCE (git submodule in third_party/JUCE)
add_subdirectory(third_party/JUCE)

# Core sources shared by both plugins and the tests (see core/CoreSources.cmake).
# They are compiled INTO each target, so every binary uses the same compiler
# flags and the same MSVC runtime.
include(core/CoreSources.cmake)

enable_testing()
add_subdirectory(tests)
add_subdirectory(plugins/track)
add_subdirectory(plugins/hub)
