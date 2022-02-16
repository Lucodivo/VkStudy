include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

set(LIBS vkbootstrap vma glm tinyobjloader imgui stb_image tinygltf spirv_reflect assetlib gtest_main)

add_executable(
        shader_reflect_test
        shader_reflect_test.cpp
)
target_link_libraries(shader_reflect_test ${LIBS})
add_dependencies(shader_reflect_test Shaders)

add_executable(
        playground_test
        playground_test.cpp
)
target_link_libraries(playground_test ${LIBS})

include(GoogleTest)
gtest_discover_tests(
        shader_reflect_test
        playground_test)