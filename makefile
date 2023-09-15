# Compiler and flags
NVCC = nvcc
CXX = g++
CXXFLAGS = -std=c++11 -I/opt/cuda/include -I./include -I./external/include
LDFLAGS = -lX11 -lGL -lGLEW -limgui
CUDA_LIBS = libcudadevrt.a libcudart_static.a

# Directories
SRC_DIR = src
BUILD_DIR = build
EXTERNAL_DIR = external
INCLUDE_DIR = include

# Source files
CUDA_SOURCES = $(SRC_DIR)/RenderLauncherCUDA.cu
CXX_SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
CXX_SOURCES += $(wildcard $(SRC_DIR)/*.c)

# Source files in the desired order
SOURCES = \
    $(SRC_DIR)/main.cpp \
    $(SRC_DIR)/StringUtils.cpp \
    $(SRC_DIR)/GridAssetUrl.cpp \
    $(SRC_DIR)/FrameBuffer.cpp \
    $(SRC_DIR)/FrameBufferHost.cpp \
    $(SRC_DIR)/Renderer.cpp \
    $(SRC_DIR)/RenderLauncher.cpp \
    $(SRC_DIR)/RenderLauncherCpuMT.cpp \
    $(SRC_DIR)/RenderLauncherC99.cpp \
    $(SRC_DIR)/GridManager.cpp \
    $(SRC_DIR)/AssetLoader.cpp \
    $(SRC_DIR)/CallbackPool.cpp \
    $(SRC_DIR)/Viewer.cpp \
    $(SRC_DIR)/FrameBufferGL.cpp \
    $(SRC_DIR)/RenderLauncherGL.cpp \
    $(SRC_DIR)/RenderLauncherCL.cpp \

# CUDA source files
CUDA_SOURCES = $(SRC_DIR)/RenderLauncherCUDA.cu

# Object files
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))
CUDA_OBJECTS = $(patsubst $(SRC_DIR)/%.cu,$(BUILD_DIR)/%.o,$(CUDA_SOURCES))

# Executable
EXECUTABLE = viewer.run

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) $(CUDA_OBJECTS)
	$(CXX) $^ -o $@ $(LDFLAGS) $(CUDA_LIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cu
	$(NVCC) -c $< -o $@ --extended-lambda -use_fast_math -lineinfo -O3 -DNDEBUG $(CXXFLAGS)

clean:
	rm -rf $(BUILD_DIR)/*.o $(EXECUTABLE)

.PHONY: all clean

