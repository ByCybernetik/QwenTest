cmake_minimum_required(VERSION 3.10)
project(VulkanTriangle)

set(CMAKE_CXX_STANDARD 17)

# Find Vulkan package
find_package(Vulkan REQUIRED)

# Add executable
add_executable(vulkan_triangle main.cpp)

# Link Vulkan libraries
target_link_libraries(vulkan_triangle Vulkan::Vulkan)

# Include directories
target_include_directories(vulkan_triangle PRIVATE ${Vulkan_INCLUDE_DIRS})

# Find glslc compiler
find_program(GLSLC glslc HINTS $ENV{VULKAN_SDK}/bin)

if(NOT GLSLC)
    message(FATAL_ERROR "glslc not found. Please install Vulkan SDK.")
endif()

# Create output directory for SPIR-V shaders
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shaders)

# Compile vertex shader to SPIR-V
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/vert.spv
    COMMAND ${GLSLC} ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.vert -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/vert.spv
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.vert
    COMMENT "Compiling vertex shader to SPIR-V"
)

# Compile fragment shader to SPIR-V
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/frag.spv
    COMMAND ${GLSLC} ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.frag -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/frag.spv
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.frag
    COMMENT "Compiling fragment shader to SPIR-V"
)

# Add custom target to build shaders
add_custom_target(shaders DEPENDS 
    ${CMAKE_CURRENT_BINARY_DIR}/shaders/vert.spv
    ${CMAKE_CURRENT_BINARY_DIR}/shaders/frag.spv
)

# Make sure shaders are compiled before building the main executable
add_dependencies(vulkan_triangle shaders)
