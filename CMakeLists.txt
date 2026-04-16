cmake_minimum_required(VERSION 3.16)
project(VulkanTriangle VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

# Vulkan library sources
set(VULKAN_SOURCES
    vulkan/vulkan_device.cpp
    vulkan/vulkan_swapchain.cpp
    vulkan/vulkan_pipeline.cpp
    vulkan/vulkan_command_buffer.cpp
)

set(VULKAN_HEADERS
    vulkan/vulkan_types.h
    vulkan/vulkan_renderer.h
)

# Add executable for test application
add_executable(${PROJECT_NAME} test/main.cpp ${VULKAN_SOURCES} ${VULKAN_HEADERS})

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE 
    Vulkan::Vulkan 
    glfw
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR})

# Shader compilation
set(SHADER_SOURCE_DIR "${CMAKE_SOURCE_DIR}/shaders")
set(SHADER_BINARY_DIR "${CMAKE_BINARY_DIR}")

# Create shaders directory in build if it doesn't exist
file(MAKE_DIRECTORY ${SHADER_BINARY_DIR}/shaders)

# Find shader files
file(GLOB SHADER_FILES "${SHADER_SOURCE_DIR}/*.vert" "${SHADER_SOURCE_DIR}/*.frag")

# Compile each shader
foreach(SHADER_FILE ${SHADER_FILES})
    get_filename_component(SHADER_NAME ${SHADER_FILE} NAME)
    set(SHADER_OUTPUT "${SHADER_BINARY_DIR}/shaders/${SHADER_NAME}.spv")
    
    # Use glslangValidator if glslc is not available
    if(Vulkan_GLSLC_EXECUTABLE)
        set(SHADER_COMPILER ${Vulkan_GLSLC_EXECUTABLE})
        set(SHADER_COMPILE_ARGS ${SHADER_FILE} -o ${SHADER_OUTPUT})
    elseif(Vulkan_GLSLANG_VALIDATOR_EXECUTABLE)
        set(SHADER_COMPILER ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE})
        set(SHADER_COMPILE_ARGS -V ${SHADER_FILE} -o ${SHADER_OUTPUT})
    else()
        message(FATAL_ERROR "No Vulkan shader compiler found")
    endif()
    
    add_custom_command(
        OUTPUT ${SHADER_OUTPUT}
        COMMAND ${SHADER_COMPILER} ${SHADER_COMPILE_ARGS}
        DEPENDS ${SHADER_FILE}
        COMMENT "Compiling shader: ${SHADER_NAME}"
    )
    
    # Add to custom target to ensure shaders are built
    list(APPEND SHADER_OUTPUTS ${SHADER_OUTPUT})
endforeach()

# Custom target for shaders
add_custom_target(shaders ALL DEPENDS ${SHADER_OUTPUTS})

# Ensure shaders are built before the main target
add_dependencies(${PROJECT_NAME} shaders)

# Copy shaders to binary directory (already done by compilation above)
# But let's also ensure they're copied to the same directory as executable
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory 
        ${SHADER_BINARY_DIR}/shaders 
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
    COMMENT "Copying shaders to executable directory"
)
