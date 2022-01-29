# Add source to this project's executable.
add_executable(vulkan_guide
    vk_study.cpp)

set(LIBS vkbootstrap vma glm tinyobjloader imgui stb_image tinygltf spirv_reflect)

set_property(TARGET vulkan_guide PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:vulkan_guide>")

target_include_directories(vulkan_guide PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(vulkan_guide ${LIBS})

target_link_libraries(vulkan_guide Vulkan::Vulkan sdl2)

add_dependencies(vulkan_guide Shaders)

# testing
include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(
        shader_reflect_test
        tests/shader_reflect_test.cpp
)
target_link_libraries(shader_reflect_test ${LIBS} gtest_main)

include(GoogleTest)
gtest_discover_tests(shader_reflect_test)
add_dependencies(shader_reflect_test Shaders)
