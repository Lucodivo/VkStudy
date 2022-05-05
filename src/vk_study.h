#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <functional>
#include <unordered_map>

#define NOMINMAX
#include <windows.h>

#include <vulkan/vulkan.h>

#include <SDL.h>
#include <SDL_vulkan.h>

// TODO: remove? Still used in material.h
// TODO: Maybe asset baker can use SPIRV CROSS to pull out what is needed?
#include <spirv_reflect.h>

#include <imgui.h>
#include <backends/imgui_impl_sdl.h>
#include <backends/imgui_impl_vulkan.h>

#include <VkBootstrap.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "types.h"
#include "vk_types.h"
#include "window_manager.h"
#include "noop_math/noop_math.h"
using namespace noop;
#include "util.h"
#include "cstring_ring_buffer.h"
#include "camera.h"
#include "vk_util.h"
#include "vk_initializers.h"
#include "vk_textures.h"
#include "vk_mesh.h"
#include "materials.h"
#include "vk_pipeline_builder.h"
#include "vk_imgui.h"
#include "vk_engine.h"

#include "asset_loader.h"
#include "texture_asset.h"
#include "mesh_asset.h"
#include "material_asset.h"
#include "prefab_asset.h"

#include "baked_assets.h"

#include "camera.cpp"
#include "util.cpp"
#include "windows_util.cpp"
#include "vk_util.cpp"
#include "vk_initializers.cpp"
#include "vk_textures.cpp"
#include "vk_mesh.cpp"
#include "vk_pipeline_builder.cpp"
#include "materials.cpp"
#include "vk_engine.cpp"
#include "vk_imgui.cpp"