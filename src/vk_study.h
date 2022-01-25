#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <functional>
#include <unordered_map>

#include <stdint.h>

#include <windows.h>

#include <vulkan/vulkan.h>

#include <SDL.h>
#include <SDL_vulkan.h>

#include <stb_image.h>
#include <tiny_obj_loader.h>
#include <tiny_gltf.h>

#include <spirv_reflect.h>

#include <glm/gtx/transform.hpp>
#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_vulkan.h>

#include <VkBootstrap.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "types.h"
#include "vk_types.h"
#include "util.h"
#include "camera.h"
#include "vk_util.h"
#include "vk_initializers.h"
#include "vk_textures.h"
#include "vk_mesh.h"
#include "materials.h"
#include "vk_pipeline_builder.h"
#include "vk_engine.h"

#include "camera.cpp"
#include "util.cpp"
#include "vk_util.cpp"
#include "vk_initializers.cpp"
#include "vk_textures.cpp"
#include "vk_mesh.cpp"
#include "vk_pipeline_builder.cpp"
#include "vk_engine.cpp"
#include "material.cpp"