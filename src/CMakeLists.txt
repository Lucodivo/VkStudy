# Add source to this project's executable.
add_executable(vulkan_guide
    vk_study.cpp)

add_executable(testing_ground
    testing_ground/testing_ground.cpp)

set(LIBS vkbootstrap vma glm tinyobjloader imgui stb_image tinygltf spirv_reflect)

set_property(TARGET vulkan_guide PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:vulkan_guide>")

target_include_directories(vulkan_guide PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(vulkan_guide ${LIBS})
target_link_libraries(testing_ground ${LIBS})

target_link_libraries(vulkan_guide Vulkan::Vulkan sdl2)

add_dependencies(vulkan_guide Shaders)
add_dependencies(testing_ground Shaders)
