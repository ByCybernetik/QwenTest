cmake_minimum_required(VERSION 3.10)
project(VulkanTriangle)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

# Add executable
add_executable(triangle main.cpp)

# Link libraries
target_link_libraries(triangle Vulkan::Vulkan glfw)

# Include directories
target_include_directories(triangle PRIVATE ${Vulkan_INCLUDE_DIRS})
