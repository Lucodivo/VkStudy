## Purpose

This project currently serves as my own personal project for studying Vulkan.

### Building (⚠IN PROGRESS⚠)
- This project uses a unity build system. All `#includes` are in `vk_study.h` and most of the project is compiled all at
  once through `vk_study.cpp`.
- `sdl2_DIR` environment variable in third_party/CMakeLists.txt must be set to 
  SDL2 library path (ex: "C:/developer/dependencies/libs/SDL2-2.0.18").

### Running (⚠IN PROGRESS⚠)
- Working directory should be the root directory of the project.
- `SDL2.dll` must be placed in same directory as `vk_study.exe`.

### Example Render

This render demonstrates the current capabilities of drawing ~36,000 cubes with a ShaderToy-esque background.

![35937 cubes example picture](https://raw.githubusercontent.com/Lucodivo/RepoSampleImages/master/VulkanStudy/35937_cubes.png)

#### Dependencies
- [Dear Imgui](https://github.com/ocornut/imgui) for debugging GUIs
- [stb (image/image_write)](https://github.com/nothings/stb) for loading image files
- [syoyo's tinygltf](https://github.com/syoyo/tinygltf) for loading 3D files (gltf/glb)
- [tinyobjloader](https://github.com/tinyobjloader/tinyobjloader) for loading 3D files (obj)
- [vk-bootstrap](https://github.com/charles-lunarg/vk-bootstrap) for simplifying Vulkan initialization
- [VulkanMemoryAllocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) for simplifying the management of Vulkan images/buffers
- [LZ4](https://github.com/lz4/lz4) for compressing/decompressing custom asset files
- [nlohmann's json](https://github.com/nlohmann/json) for custom asset file headers
- [SPIRV-Reflect](https://github.com/KhronosGroup/SPIRV-Reflect) for reflection on GLSL shaders

### Special Thanks
- [vblanco20-1](https://github.com/vblanco20-1) and their fantastic [Vulkan Guide](https://vkguide.dev/)
- [Sascha Willems](https://github.com/SaschaWillems) and his amazing Vulkan education contributions everywhere you look.
- [Graphics Programming Discord](https://discord.com/invite/6mgNGk7)
- [/r/vulkan](https://www.reddit.com/r/vulkan/)