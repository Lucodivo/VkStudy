# vk_study
add_executable(vk_study vk_study.cpp)
target_include_directories(vk_study PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
set(LIBS vkbootstrap vma imgui spirv_reflect assetlib json lz4 noop_math Vulkan::Vulkan sdl2 inih)
target_link_libraries(vk_study ${LIBS})

add_dependencies(vk_study Shaders)