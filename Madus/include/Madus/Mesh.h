// Copyright Lukas Licon 2025, All Rights Reservered.

#pragma once
#include <cstdint>

struct GpuMesh {
    unsigned vao=0, vbo=0, ibo=0;
    uint32_t indexCount=0;
};

GpuMesh CreateBoxUnit();      // 1x1x1 centered at origin
GpuMesh CreatePlane(float size=20.f);
void    DestroyMesh(GpuMesh& m);
