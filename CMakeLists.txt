cmake_minimum_required(VERSION 3.14)
project(voxel_engine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory(external/glfw)
add_subdirectory(external/glm)
add_subdirectory(external/assimp)

add_library(glad external/glad/src/glad.c)
target_include_directories(glad PUBLIC external/glad/include)

add_executable(${PROJECT_NAME} src/main.cpp)

target_sources(
    ${PROJECT_NAME} PRIVATE
    src/main.cpp)

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/res" 
     DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")

target_include_directories(${PROJECT_NAME} PUBLIC external/glm/glm)
target_include_directories(${PROJECT_NAME} PUBLIC external/assimp/include)
target_include_directories(${PROJECT_NAME} PUBLIC external)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw glad glm::glm opengl32 assimp) 

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

if (MSVC) 
    target_compile_options(${PROJECT_NAME} PRIVATE /Zc:__cplusplus /permissive-)
endif()