# vk_study
add_executable(vk_study vk_study.cpp)
target_include_directories(vk_study PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
set(LIBS vkbootstrap vma imgui spirv_reflect assetlib noop_math Vulkan::Vulkan sdl2)
target_link_libraries(vk_study ${LIBS})
set_property(TARGET vk_study PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:vk_study>")

add_dependencies(vk_study Shaders)