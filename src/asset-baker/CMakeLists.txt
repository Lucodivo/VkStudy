# Add source to this project's executable.
add_executable (asset-baker
"asset_main.cpp")

set_property(TARGET asset-baker PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:extra>")

target_include_directories(asset-baker PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

#set(INCL_DIR "C:/developer/dependencies/include")
#include_directories(${INCL_DIR})

set(LIBS tinyobjloader stb_image tinygltf assetlib json lz4) # nvtt assimp
target_link_libraries(asset-baker ${LIBS})