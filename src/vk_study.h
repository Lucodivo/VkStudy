#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <functional>
#include <unordered_map>

#include <stdint.h>

#define NOMINMAX
#include <windows.h>

#include <vulkan/vulkan.h>

#include <SDL.h>
#include <SDL_vulkan.h>

// TODO: remove? Still used in material.h
// TODO: Maybe asset baker can use SPIRV CROSS to pull out what is needed?
#include <spirv_reflect.h>

#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_vulkan.h>

#include <VkBootstrap.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "assetlib/asset_loader.h"
#include "assetlib/texture_asset.h"

#include "types.h"
#include "vk_types.h"
#include "noop_math.h"
#include "util.h"
#include "cstring_ring_buffer.h"
#include "camera.h"
#include "vk_util.h"
#include "vk_initializers.h"
#include "vk_textures.h"
#include "vk_mesh.h"
#include "materials.h"
#include "vk_pipeline_builder.h"
#include "vk_engine.h"

#include "asset_loader.h"
#include "texture_asset.h"
#include "mesh_asset.h"
#include "material_asset.h"
#include "prefab_asset.h"

#include "baked_assets.h"

#include "camera.cpp"
#include "util.cpp"
#include "vk_util.cpp"
#include "vk_initializers.cpp"
#include "vk_textures.cpp"
#include "vk_mesh.cpp"
#include "vk_pipeline_builder.cpp"
#include "imgui_util.cpp"
#include "material.cpp"
#include "vk_engine.cpp"