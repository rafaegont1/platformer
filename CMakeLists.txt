# Set the cmake minimum version
cmake_minimum_required(VERSION 3.5)

# Define the project name
project(platformer)

# Generate symbols for IDE indexer
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default to C11
set(CMAKE_C_STANDARD 11)

# Find required packages
find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)

# Set source files
file(GLOB SOURCES "src/*.c")

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Add all headers files under the include directory
target_include_directories(${PROJECT_NAME} PRIVATE include)

# Link libraries to executable
target_link_libraries(${PROJECT_NAME} PUBLIC SDL2_ttf::SDL2_ttf SDL2::SDL2 m)

# Add compilation flags
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)

# Enable better debugging information
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${PROJECT_NAME} PRIVATE -g -O0)
endif()
