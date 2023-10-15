# OpenVDB Volume Tracing using OpenGL and CUDA

This project explores implementing real-time path tracing techniques using CUDA to render volumetric data stored in OpenVDB format. Path tracing is a physically-based rendering method known for its ability to simulate realistic lighting and material interactions. OpenVDB is a popular open-source library for efficiently storing and manipulating volumetric data, making it an ideal choice for rendering complex 3D volumes.

## Demo
Checkout whole demonstration here :-P
[Watch the Demo](https://youtu.be/YiZO55t-wvk?si=uJmNzf8Mn1PmFx7o)

## Features

- Loading OpenVDB volumes.
- Support For Creating procedural volumes at runtime.
- Rendering Options: Lighting, Shadows, Background.
- Material Types: Level Set, Fog Volume Tacer, Grid, Point Fast, Black Body Volume Path Tracer, Fog Volume Fast.
- Multiple Platforms: CPU, CPU MultiThreaded, CUDA, OpenGL Compute Shader (In Progress), OpenCL (In Progress).

Technical Details:
========================

- HPP Technology: CUDA
- Rendering API: OpenGL
- Programming Language: C++ 
- Operating System: Manjaro Linux
- CPU: Ryzen 7 3700x
- GPU: NVidia RTX 2070 Super

External Libraries:
========================
- ImGUI (X11 Linux Backend ported by Me).
- OpenVDB
- CUDA
- XWindows

References (Books and Links) :
========================

- CUDA By Example
- OpenVDB Documentation
- A GPU-Friendly and Portable VDB Data Structure For Real-Time Rendering and Simulation
- Ray Tracing In One Weekend (Series)

- Ray Tracing Series: The  Cherno
- Interactive Volume Rendering: Cem Yuksel



