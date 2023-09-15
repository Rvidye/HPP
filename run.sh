#!/bin/bash

nvcc -c src/RenderLauncherCUDA.cu --extended-lambda -use_fast_math -lineinfo -O3 -DNDEBUG -std=c++11 -I include/

g++ -I./external/include -I/opt/cuda/include -I./include -c src/main.cpp src/RenderLauncherC99impl.c src/StringUtils.cpp src/GridAssetUrl.cpp src/FrameBuffer.cpp src/FrameBufferHost.cpp src/Renderer.cpp src/RenderLauncher.cpp src/RenderLauncherCpuMT.cpp src/RenderLauncherC99.cpp src/GridManager.cpp src/AssetLoader.cpp src/CallbackPool.cpp src/Viewer.cpp src/FrameBufferGL.cpp src/RenderLauncherGL.cpp src/RenderLauncherCL.cpp

g++ *.o -o viewer.run -lX11 -lGL -lGLEW -limgui libcudadevrt.a libcudart_static.a 
