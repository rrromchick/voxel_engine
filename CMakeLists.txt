cmake_minimum_required(VERSION 3.16)
project(voxel_engine CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

find_package(OpenGL REQUIRED)
add_subdirectory(external/glfw)
add_subdirectory(external/glm)

add_library(glad external/glad/src/glad.c)
target_include_directories(glad PUBLIC external/glad/include)

add_library(imgui STATIC
    external/imgui/imgui.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/backends/imgui_impl_glfw.cpp
    external/imgui/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    external/imgui
    external/imgui/backends
)
target_link_libraries(imgui PUBLIC glfw glad OpenGL::GL)

add_library(engine_core STATIC
    src/gfx/Texture.cpp
    src/gfx/Mesh.cpp
    src/gfx/VoxelRenderer.cpp
    src/gfx/LineBatch.cpp
    src/voxels/Chunk.cpp
    src/voxels/WorldGenerator.cpp
    src/voxels/Level.cpp
    src/lighting/Lightmap.cpp
    src/lighting/LightSolver.cpp
    src/lighting/Lighting.cpp
    src/files/WorldFiles.cpp
    src/physics/PhysicsSolver.cpp
    src/state/StateGame.cpp
    src/Player.cpp
    src/gfx/VoxelModel.cpp
)

target_include_directories(engine_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gfx
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lighting
    ${CMAKE_CURRENT_SOURCE_DIR}/src/util
    ${CMAKE_CURRENT_SOURCE_DIR}/src/voxels
    ${CMAKE_CURRENT_SOURCE_DIR}/src/window
    ${CMAKE_CURRENT_SOURCE_DIR}/src/files
    ${CMAKE_CURRENT_SOURCE_DIR}/src/physics
    ${CMAKE_CURRENT_SOURCE_DIR}/src/state
    ${CMAKE_CURRENT_SOURCE_DIR}/src/network
    external
    external/glm/glm
)

target_link_libraries(engine_core PUBLIC
    glfw
    glad
    glm::glm
    OpenGL::GL
    imgui
    $<$<PLATFORM_ID:Windows>:ws2_32>
)

if(MSVC)
    target_compile_options(engine_core PRIVATE /Zc:__cplusplus /permissive-)
endif()

add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE engine_core)

add_executable(client src/client.cpp)
target_link_libraries(client PRIVATE engine_core)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/res"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/res"
)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-g -O0)
    # Add allow-multiple-definition to ignore libstdc++ / typeinfo inline collisions
    add_link_options(-Wl,-pdb= -Wl,--allow-multiple-definition)
endif()
