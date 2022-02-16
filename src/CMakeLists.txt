# Add source to this project's executable.
add_executable(vk_study
    vk_study.cpp)

set(LIBS vkbootstrap vma glm tinyobjloader imgui stb_image tinygltf spirv_reflect assetlib)

set_property(TARGET vk_study PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:vk_study>")

target_include_directories(vk_study PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(vk_study ${LIBS})

target_link_libraries(vk_study Vulkan::Vulkan sdl2)

add_dependencies(vk_study Shaders)