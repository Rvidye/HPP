/// Copyright Contributors to the OpenVDB Project
// SPDX-License-Identifier: MPL-2.0

/*!
	\file RenderLauncherC.cpp
	\brief Implementation of C99-platform Grid renderer.
*/

#include <iostream>
#include <sstream>
#include <fstream>
#include "../include/RenderLauncherImpl.h"
#include "../include/FrameBuffer.h"
#include <nanovdb/util/NodeManager.h>

extern "C" {
#define VALUETYPE float
#define SIZEOF_VALUETYPE 4
#include "AgnosticNanoVDB.h"
#include "code/renderCommon.h"
#include "AgnosticNanoVDB.h"
#include "code/renderLevelSet.c"
#include "code/renderFogVolume.c"
}

void launchRender(int method, int width, int height, vec4* imgPtr, const nanovdb_Node0_float* node0Level, const nanovdb_Node1_float* node1Level, const nanovdb_Node2_float* node2Level, const nanovdb_RootData_float* rootData, const nanovdb_RootData_Tile_float* rootDataTiles, const nanovdb_GridData* gridData, const ArgUniforms* uniforms)
{
    const int blockSize = 16;
    const int nBlocksX = (width + (blockSize - 1)) / blockSize;
    const int nBlocksY = (height + (blockSize - 1)) / blockSize;
    const int nBlocks = nBlocksX * nBlocksY;

    for (int blockIndex = 0; blockIndex < nBlocks; ++blockIndex) {
        const int blockOffsetX = blockSize * (blockIndex % nBlocksX);
        const int blockOffsetY = blockSize * (blockIndex / nBlocksY);

        if (method == CNANOVDB_RENDERMETHOD_LEVELSET) {
            for (int iy = 0; iy < blockSize; ++iy) {
                for (int ix = 0; ix < blockSize; ++ix) {
                    renderLevelSet(
                        CNANOVDB_MAKE_IVEC2(ix + blockOffsetX, iy + blockOffsetY),
                        imgPtr,
                        node0Level,
                        node1Level,
                        node2Level,
                        rootData,
                        rootDataTiles,
                        gridData,
                        *uniforms);
                }
            }
        } else if (method == CNANOVDB_RENDERMETHOD_FOG_VOLUME) {
            for (int iy = 0; iy < blockSize; ++iy) {
                for (int ix = 0; ix < blockSize; ++ix) {
                    renderFogVolume(
                        CNANOVDB_MAKE_IVEC2(ix + blockOffsetX, iy + blockOffsetY),
                        imgPtr,
                        node0Level,
                        node1Level,
                        node2Level,
                        rootData,
                        rootDataTiles,
                        gridData,
                        *uniforms);
                }
            }
        } else {
            for (int iy = 0; iy < blockSize; ++iy) {
                for (int ix = 0; ix < blockSize; ++ix) {
                    int gx = (blockOffsetX + ix);
                    int gy = (blockOffsetY + iy);
                    if (gx >= width || gy >= height)
                        continue;
                    *((imgPtr) + gx + gy * width) = CNANOVDB_MAKE_VEC4((float)gx / width, (float)gy / height, 1, 1);
                }
            }
        }
    }
}

bool RenderLauncherC99::render(MaterialClass method, int width, int height, FrameBufferBase* imgBuffer, int numAccumulations, int /*numGrids*/, const GridRenderParameters* grids, const SceneRenderParameters& sceneParams, const MaterialParameters& materialParams, RenderStatistics* stats)
{
    if (grids[0].gridHandle == nullptr)
        return false;

    auto& gridHdl = *reinterpret_cast<const nanovdb::GridHandle<>*>(grids[0].gridHandle);

    float* imgPtr = (float*)imgBuffer->map((numAccumulations > 0) ? FrameBufferBase::AccessType::READ_WRITE : FrameBufferBase::AccessType::WRITE_ONLY);
    if (!imgPtr) {
        return false;
    }

    using ClockT = std::chrono::high_resolution_clock;
    auto t0 = ClockT::now();

    // prepare data...

    if (gridHdl.gridMetaData()->gridType() == nanovdb::GridType::Float) {
        auto grid = gridHdl.grid<float>();

        using GridT = nanovdb::NanoGrid<float>;
        using TreeT = GridT::TreeType;
        using RootT = TreeT::RootType;
        using Node2T = RootT::ChildNodeType;
        using Node1T = Node2T::ChildNodeType;
        using Node0T = Node1T::ChildNodeType;

        auto mgr = nanovdb::createNodeMgr(*grid);

        auto* node0Level = mgr.leaf(0);
        auto* node1Level = mgr.lower(0);
        auto* node2Level = mgr.upper(0);
        auto* rootData = &grid->tree().root();
        auto* gridData = grid;

        nanovdb::Vec3f cameraP = sceneParams.camera.P();
        nanovdb::Vec3f cameraU = sceneParams.camera.U();
        nanovdb::Vec3f cameraV = sceneParams.camera.V();
        nanovdb::Vec3f cameraW = sceneParams.camera.W();

        // launch render...

        ArgUniforms uniforms;

        uniforms.width = width;
        uniforms.height = height;
        uniforms.numAccumulations = numAccumulations;

        uniforms.useShadows = sceneParams.useShadows;
        uniforms.useGroundReflections = sceneParams.useGroundReflections;
        uniforms.useLighting = sceneParams.useLighting;
        uniforms.useOcclusion = materialParams.useOcclusion;
        uniforms.volumeDensityScale = materialParams.volumeDensityScale;

        uniforms.samplesPerPixel = sceneParams.samplesPerPixel;
        uniforms.useBackground = sceneParams.useBackground;
        uniforms.tonemapWhitePoint = sceneParams.tonemapWhitePoint;
        uniforms.groundHeight = sceneParams.groundHeight;
        uniforms.groundFalloff = sceneParams.groundFalloff;
        uniforms.cameraPx = cameraP[0];
        uniforms.cameraPy = cameraP[1];
        uniforms.cameraPz = cameraP[2];
        uniforms.cameraUx = cameraU[0];
        uniforms.cameraUy = cameraU[1];
        uniforms.cameraUz = cameraU[2];
        uniforms.cameraVx = cameraV[0];
        uniforms.cameraVy = cameraV[1];
        uniforms.cameraVz = cameraV[2];
        uniforms.cameraWx = cameraW[0];
        uniforms.cameraWy = cameraW[1];
        uniforms.cameraWz = cameraW[2];
        uniforms.cameraAspect = sceneParams.camera.aspect();
        uniforms.cameraFovY = sceneParams.camera.fov();

        launchRender((int)method, width, height, (vec4*)imgPtr, (nanovdb_Node0_float*)node0Level, (nanovdb_Node1_float*)node1Level, (nanovdb_Node2_float*)node2Level, (nanovdb_RootData_float*)rootData, (nanovdb_RootData_Tile_float*)(rootData + 1), (nanovdb_GridData*)gridData, &uniforms);
    }

    imgBuffer->unmap();

    if (stats) {
        auto t1 = ClockT::now();
        stats->mDuration = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count() / 1000.f;
    }

    return true;
}
