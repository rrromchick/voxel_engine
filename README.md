Voxel Engine
    
    A C++23 modular voxel engine built with OpenGL, GLFW, ImGui, GLM, and SQLite3.

Prerequisites:

    Before building, make sure all git submodules in the external/ directory are initialized and updated:

    git submodule update --init --recursive

Building on Windows

    Method 1: MinGW (GCC / G++)

        Ensure MinGW-w64 and CMake are installed and added to your system PATH.

        Configure the project in Release mode for maximum performance:
            cmake -B build -S . -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++

        Build the project executables using parallel jobs:
            cmake --build build -j

        Run the application:
            .\build\voxel_engine.exe for server and .\build\client.exe for client

    Method 2: Visual Studio 2022 (or whatever you are using)

        Ensure Visual Studio 2022 is installed with the "Desktop development with C++" workload.

        Open the Developer Command Prompt for VS 2022.

        Configure the CMake project:
            cmake -B build -S . -G "Visual Studio 17 2022" -A x64

        Build in Release configuration:
            cmake --build build --config Release

        Run the application:
            .\build\Release\voxel_engine.exe and .\build\client.exe for client

Building on Linux

    Ensure you have a modern GCC (13+) or Clang compiler supporting C++23, CMake, OpenGL drivers, and X11/Wayland development headers installed.

    Install Dependencies (Debian / Ubuntu based)

        sudo apt update
        sudo apt install build-essential cmake libgl1-mesa-dev xorg-dev

    Build Steps

        Configure the project:
            cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

        Build the project:
            cmake --build build -j$(nproc)

        Run the application:
            ./build/voxel_engine for server and ./build/client for client
