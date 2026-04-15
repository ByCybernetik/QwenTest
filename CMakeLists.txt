cmake_minimum_required(VERSION 3.10)
project(VulkanTriangle)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

# Add executable
add_executable(vulkan_triangle src/main.cpp)

# Link libraries
target_link_libraries(vulkan_triangle Vulkan::Vulkan glfw)

# Include directories
target_include_directories(vulkan_triangle PRIVATE ${Vulkan_INCLUDE_DIRS})

# Find glslc compiler (from Vulkan SDK) or glslangValidator
find_program(GLSLC glslc HINTS $ENV{VULKAN_SDK}/bin)
if(NOT GLSLC)
    # Fallback to glslangValidator if glslc is not available
    find_program(GLSLANG_VALIDATOR glslangValidator)
    if(GLSLANG_VALIDATOR)
        set(GLSLC ${GLSLANG_VALIDATOR})
        set(USE_GLSLANG TRUE)
    endif()
endif()

if(NOT GLSLC)
    message(FATAL_ERROR "Neither glslc nor glslangValidator found! Please install Vulkan SDK or glslang-tools.")
endif()

# Create shaders directory in build folder
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)

# Compile vertex shader
if(USE_GLSLANG)
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/shaders/vert.spv
        COMMAND ${GLSLC} -V ${CMAKE_SOURCE_DIR}/shaders/vert.glsl -o ${CMAKE_BINARY_DIR}/shaders/vert.spv -S vert
        DEPENDS ${CMAKE_SOURCE_DIR}/shaders/vert.glsl
        COMMENT "Compiling vertex shader with glslangValidator"
    )

    # Compile fragment shader
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/shaders/frag.spv
        COMMAND ${GLSLC} -V ${CMAKE_SOURCE_DIR}/shaders/frag.glsl -o ${CMAKE_BINARY_DIR}/shaders/frag.spv -S frag
        DEPENDS ${CMAKE_SOURCE_DIR}/shaders/frag.glsl
        COMMENT "Compiling fragment shader with glslangValidator"
    )
else()
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/shaders/vert.spv
        COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/shaders/vert.glsl -o ${CMAKE_BINARY_DIR}/shaders/vert.spv
        DEPENDS ${CMAKE_SOURCE_DIR}/shaders/vert.glsl
        COMMENT "Compiling vertex shader with glslc"
    )

    # Compile fragment shader
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/shaders/frag.spv
        COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/shaders/frag.glsl -o ${CMAKE_BINARY_DIR}/shaders/frag.spv
        DEPENDS ${CMAKE_SOURCE_DIR}/shaders/frag.glsl
        COMMENT "Compiling fragment shader with glslc"
    )
endif()

# Add custom target for shaders
add_custom_target(shaders ALL
    DEPENDS 
        ${CMAKE_BINARY_DIR}/shaders/vert.spv
        ${CMAKE_BINARY_DIR}/shaders/frag.spv
)

# Ensure shaders are built before the main executable
add_dependencies(vulkan_triangle shaders)

# Copy shaders to binary directory (same location as executable)
add_custom_command(TARGET vulkan_triangle POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_BINARY_DIR}/shaders
        $<TARGET_FILE_DIR:vulkan_triangle>/shaders
    COMMENT "Copying shaders to executable directory"
)
