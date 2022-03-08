find_package(Vulkan REQUIRED)

# VulkanMemoryAllocator
add_library(vma INTERFACE)
target_include_directories(vma INTERFACE vma) # vma is a header only libs so we only need the include path

# vk-bootstrap
add_library(vkbootstrap STATIC)
target_sources(vkbootstrap PRIVATE
        vkbootstrap/VkBootstrap.h
        vkbootstrap/VkBootstrap.cpp
)
target_include_directories(vkbootstrap PUBLIC vkbootstrap)
target_link_libraries(vkbootstrap PUBLIC Vulkan::Vulkan $<$<BOOL:UNIX>:${CMAKE_DL_LIBS}>)

# stb
add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE stb_image)

# nlohmann json
add_library(json INTERFACE)
target_include_directories(json INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/nlohmann_json" )

# tinygltf
add_library(tinygltf STATIC)
target_sources(tinygltf PRIVATE
        tinygltf/tiny_gltf.h
        tinygltf/tiny_gltf.cc
)
target_include_directories(tinygltf PUBLIC
        tinygltf
        stb_image
        nlohmann_json
)

# tinyobjloader
add_library(tinyobjloader STATIC)
target_sources(tinyobjloader PRIVATE
        tinyobjloader/tiny_obj_loader.h
        tinyobjloader/tiny_obj_loader.cc
)
target_include_directories(tinyobjloader PUBLIC tinyobjloader)

# SPIR-V Reflect
add_library(spirv_reflect STATIC)
target_sources(spirv_reflect PRIVATE
        spirv_reflect/spirv_reflect.h
        spirv_reflect/spirv_reflect.c
)
target_include_directories(spirv_reflect PUBLIC
        spirv_reflect
)

# LZ4
add_library(lz4 STATIC)
target_sources(lz4 PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/lz4/lz4.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/lz4/lz4.c"
)
target_include_directories(lz4 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/lz4" )

# DearImgui
add_library(imgui STATIC)
target_include_directories(imgui PUBLIC imgui)
target_sources(imgui PRIVATE
        imgui/imgui.h
        imgui/imgui.cpp

        imgui/imgui_demo.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_widgets.cpp
        imgui/imgui_tables.cpp

        imgui/backends/imgui_impl_vulkan.cpp
        imgui/backends/imgui_impl_sdl.cpp
        )
target_link_libraries(imgui PUBLIC Vulkan::Vulkan sdl2)

# SDL2
add_library(sdl2 INTERFACE)
set(sdl2_DIR "SDL_PATH" CACHE PATH "Path to SDL2")
target_link_libraries(sdl2 INTERFACE SDL2 SDL2main)

if (WIN32)
target_include_directories(sdl2 INTERFACE ${sdl2_DIR}/include ${sdl2_image_DIR}/include)
target_link_directories(sdl2 INTERFACE ${sdl2_DIR}/lib/x64 ${sdl2_image_DIR}/lib/x64)
elseif (UNIX)
target_include_directories(sdl2 INTERFACE ${sdl2_DIR}/include/SDL2 ${sdl2_image_DIR}/include/SDL2)
target_link_directories(sdl2 INTERFACE ${sdl2_DIR}/lib ${sdl2_image_DIR}/lib)
endif()

