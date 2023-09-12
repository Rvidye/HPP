#!/bin/bash

nvcc -I/home/rushi/Code/openvdb-feature-nanovdb/nanovdb/nanovdb/external/glad/include -c src/RenderLauncherCUDA.cu --extended-lambda -use_fast_math -lineinfo -O3 -DNDEBUG -std=c++11

g++ -DNANOVDB_USE_CUDA -DNANOVDB_USE_GLAD -DNANOVDB_USE_GLFW -DNANOVDB_USE_IMGUI -DNANOVDB_USE_OPENGL -c src/main.cpp src/RenderLauncherC99impl.c src/StringUtils.cpp src/GridAssetUrl.cpp src/FrameBuffer.cpp src/FrameBufferHost.cpp src/Renderer.cpp src/RenderLauncher.cpp src/RenderLauncherCpuMT.cpp src/RenderLauncherC99.cpp src/GridManager.cpp src/AssetLoader.cpp src/CallbackPool.cpp src/Viewer.cpp src/FrameBufferGL.cpp src/RenderLauncherGL.cpp src/RenderLauncherCL.cpp  -DNANOVDB_USE_CUDA -I ./include -I/home/rushi/Code/openvdb-feature-nanovdb/nanovdb/nanovdb/nanovdb -I/home/rushi/Code/openvdb-feature-nanovdb/nanovdb/nanovdb/external -I/home/rushi/Code/openvdb-feature-nanovdb/nanovdb/nanovdb/build/_deps/imgui-src -I/home/rushi/Code/openvdb-feature-nanovdb/nanovdb/nanovdb/external/glad/include -I/home/rushi/Code/openvdb-feature-nanovdb/nanovdb/nanovdb/. -I/home/rushi/Code/openvdb-feature-nanovdb/nanovdb/nanovdb/./nanovdb -I/home/rushi/Code/openvdb-feature-nanovdb/nanovdb/nanovdb/build/_deps/glfw-src/include -I/opt/cuda/include 

g++ *.o -o viewer.run -lX11 -lGL -lGLEW -lglfw libglad.a libimgui.a libcudadevrt.a libcudart_static.a 
