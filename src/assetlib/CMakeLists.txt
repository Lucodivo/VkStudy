# Add source to this project's executable.
add_library (assetlib STATIC
        "asset_loader.cpp"
        "texture_asset.cpp"
        "mesh_asset.cpp"
        "material_asset.cpp"
        "prefab_asset.cpp"
)

target_include_directories(assetlib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(assetlib PRIVATE json lz4)