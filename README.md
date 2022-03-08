## Purpose

Project for studying Vulkan and a general rendering engine playground for myself.

### Building (⚠IN PROGRESS⚠)
- This project has a few moving pieces to it:
  - `vk_study`: This is the executable that actually contains the Vulkan rendering engine. It mostly uses a unity build 
  system. Meaning, all `#includes` are in a single place (`vk_study.h`) and most of the project is compiled all at once 
  through the compilation of `vk_study.cpp` alone.
  - `asset_baker`: This executable is responsible for massaging assets in external formats into a format that the 
  `vk_study` executable can then directly load into memory. The idea is that we "bake" the assets into a format that is
  exactly how we want to consume them. This avoids the need to parse or arrange assets during runtime of `vk_study`. This
  executable also partitions the information about external file formats away from the rest of the project. Removing 
  dependencies from `vk_study`.
    - External formats include: .jpg, .png, .tga, .gltf, .glb, .obj
  - Some source files are compiled separately as static libraries:
    - `assetlib`: This library contains the definitions of the custom asset file formats. It defines how the structure
    of the custom assets, implements saving/compressing and loading/decompressing. This library is the communication line
    between `asset_baker` and `vk_study`.
    - `noop_math`: Custom math library originating from [NoopScenes](https://github.com/Lucodivo/NoopScenes) project
  - `sdl2_DIR` environment variable in third_party/CMakeLists.txt must be set to the SDL2 library path 
    - Ex: "C:/developer/dependencies/libs/SDL2-2.0.18"

### Running (⚠IN PROGRESS⚠)
- Ensure that the working directory when running `vk_study` or `vk_baker` is the root directory of the project.
- `vk_baker` takes two command line string arguments.
  - Argument 1: Directory of the assets
  - Argument 2: Directory of include file metadata output
  - Ex: `asset_baker.exe assets/ assets_metadata/`
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