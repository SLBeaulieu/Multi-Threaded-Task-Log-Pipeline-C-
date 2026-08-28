# ===============================================================================
#  FILE:        CMakeLists.txt
#  DESCRIPTION: Cross-platform CMake build configuration file.
#               Configures C++17 standard, locates threading libraries,
#               and builds the logger_test executable.
#   AUTHOR:     Samantha Beaulieu / Student Portfolio Project
# ===============================================================================

cmake_minimum_required(VERSION 3.14)
project(CppLoggerPipeline VERSION 1.0.0 LANGUAGES CXX)

# Enforce C++17 compiler standard across all platforms
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find native operating system threading libraries (e.g., pthread)
find_package(Threads REQUIRED)

# Make project headers in the root directory visible
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Define the build executable and its source files
add_executable(logger_test main.cpp)

# Link the required threading library to the executable
target_link_libraries(logger_test PRIVATE Threads::Threads)