
#pragma once
#include "FrameBuffer.h"

class FrameBufferHost : public FrameBufferBase
{
public:
    bool  setup(int w, int h, InternalFormat format) override;
    bool  cleanup() override;
    bool  render(int x, int y, int w, int h) override;
    void* map(AccessType access) override;
    void  unmap() override;
    void* cudaMap(AccessType access, void* streamCUDA = nullptr) override;
    void  cudaUnmap(void* streamCUDA = nullptr) override;
    void* clMap(AccessType access, void* commandQueueCL) override;
    void  clUnmap(void* commandQueueCL) override;

private:
    void* mBuffer;
};
