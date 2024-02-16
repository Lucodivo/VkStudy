cmake_minimum_required (VERSION 3.8)

project ("vk_study")

set(CMAKE_CXX_STANDARD 17)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

find_package(Vulkan REQUIRED)
find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)
find_program(SPIRV_CROSS spirv-cross HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)

# All other CMakeLists.txt directories
add_subdirectory(src)
add_subdirectory(src/assetlib)
add_subdirectory(src/asset_baker)
add_subdirectory(src/noop_math)
add_subdirectory(src/tests)
add_subdirectory(third_party)

## find all the shader files under the shaders folder
file(GLOB_RECURSE GLSL_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/shaders/*.frag"
    "${PROJECT_SOURCE_DIR}/shaders/*.vert"
    "${PROJECT_SOURCE_DIR}/shaders/*.comp"
)

## iterate each shader
foreach(GLSL ${GLSL_SOURCE_FILES})
  message(STATUS "BUILDING SHADER")
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${PROJECT_SOURCE_DIR}/shaders/spv/${FILE_NAME}.spv")
  set(SPIRV_REFLECT "${SPIRV}.json")
  message(STATUS ${GLSL})
  ##execute glslang command to compile that specific shader
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
#    COMMAND ${SPIRV_CROSS} ${SPIRV} --reflect --output ${SPIRV_REFLECT} # uncomment to generate reflection json
    DEPENDS ${GLSL})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
    build_shaders
    DEPENDS ${SPIRV_BINARY_FILES}
)