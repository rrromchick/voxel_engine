cmake_minimum_required(VERSION 3.14)
project(voxel_engine)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

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
	external/imgui/backends/imgui_impl_opengl3.cpp)

add_executable(${PROJECT_NAME} src/main.cpp)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

target_sources(
    ${PROJECT_NAME} PRIVATE src/window/Window.hpp src/window/Camera.hpp src/gfx/Shader.hpp src/gfx/Texture.hpp src/gfx/Texture.cpp
    src/gfx/VAO.hpp src/gfx/VBO.hpp src/util/Time.hpp src/Global.hpp src/state/State.hpp src/gfx/Mesh.hpp 
    src/gfx/Mesh.cpp src/gfx/VoxelRenderer.hpp src/gfx/VoxelRenderer.cpp src/voxels/Chunk.hpp src/voxels/Chunk.cpp src/voxels/Chunks.hpp
	src/voxels/Chunks.cpp src/util/File.hpp src/gfx/LineBatch.hpp src/gfx/LineBatch.cpp src/lighting/Lightmap.hpp src/lighting/Lightmap.cpp
	src/lighting/LightSolver.hpp src/lighting/LightSolver.cpp src/lighting/Lighting.hpp src/lighting/Lighting.cpp src/voxels/Block.hpp
	src/files/WorldFiles.hpp src/files/WorldFiles.cpp src/voxels/WorldGenerator.hpp src/voxels/WorldGenerator.cpp
	src/physics/Hitbox.hpp src/physics/PhysicsSolver.hpp src/physics/PhysicsSolver.cpp src/state/StateGame.hpp src/state/StateGame.cpp)

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/res" 
     DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")

target_link_libraries(imgui PUBLIC glfw glad)

target_include_directories(imgui PUBLIC
    ${CMAKE_SOURCE_DIR}/external/imgui
    ${CMAKE_SOURCE_DIR}/external/imgui/backends
    ${CMAKE_SOURCE_DIR}/external/glfw/include
    ${CMAKE_SOURCE_DIR}/external/glad/include)

target_include_directories(${PROJECT_NAME} PUBLIC external/glm/glm)
target_include_directories(${PROJECT_NAME} PUBLIC external)

target_include_directories(${PROJECT_NAME} PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${CMAKE_CURRENT_SOURCE_DIR}/src/gfx
	${CMAKE_CURRENT_SOURCE_DIR}/src/lighting
	${CMAKE_CURRENT_SOURCE_DIR}/src/util
	${CMAKE_CURRENT_SOURCE_DIR}/src/voxels
	${CMAKE_CURRENT_SOURCE_DIR}/src/window
	${CMAKE_CURRENT_SOURCE_DIR}/src/files
	${CMAKE_CURRENT_SOURCE_DIR}/src/physics
	${CMAKE_CURRENT_SOURCE_DIR}/src/state
	${CMAKE_CURRENT_SOURCE_DIR}/external/imgui
	${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends)

target_link_libraries(${PROJECT_NAME} PRIVATE glfw glad glm::glm opengl32 imgui) 

if (MSVC) 
    target_compile_options(${PROJECT_NAME} PRIVATE /Zc:__cplusplus /permissive-)
endif()