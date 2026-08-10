cmake_minimum_required(VERSION 3.16)

project(evtol_sim
    VERSION 0.1.0
    DESCRIPTION "eVTOL fleet simulation"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to Release. A debug build is a conscious choice, not something you
# should get by forgetting to pass a build type.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Both binaries land in build/bin rather than next to their own CMakeLists,
# so the documented commands to run them don't depend on the directory layout.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# The simulation lives in a library rather than directly in the executable so
# the test binary can link against it. Otherwise tests would either have to
# recompile every source file or find a way around main().
add_library(evtol_core
    src/Aircraft.cpp
    src/Rng.cpp
)

target_include_directories(evtol_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(evtol_sim src/main.cpp)
target_link_libraries(evtol_sim PRIVATE evtol_core)

enable_testing()
add_subdirectory(tests)
