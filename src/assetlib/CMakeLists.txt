# Add source to this project's executable.
add_library (assetlib STATIC
        "asset_loader.h"
        "asset_loader.cpp"
        "texture_asset.h"
        "texture_asset.cpp"
        "mesh_asset.h"
        "mesh_asset.cpp"
        "material_asset.h"
        "material_asset.cpp"
        "prefab_asset.h"
        "prefab_asset.cpp"
#        "texture_format.incl"
#        "compression_mode.incl"
#        "vertex_format.incl"
#        "transparencyMode.incl"
)

target_include_directories(assetlib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(assetlib PRIVATE json lz4)