// Copyright Lukas Licon 2025, All Rights Reserved.

#pragma once

#include "Madus/Math.h"
#include <cmath>

struct Camera {
    // Lens
    float FovY  = 60.f * (float)MADUS_PI / 180.f; // radians
    float NearZ = 0.05f;
    float FarZ  = 500.f;

    // Pose / orientation
    Vec3  Pos{0, 1.7f, 6};
    float Yaw   = 0.f;   // radians
    float Pitch = 0.f;   // radians

    // Basis
    Vec3 Forward() const {
        float cy = std::cos(Yaw), sy = std::sin(Yaw);
        float cp = std::cos(Pitch), sp = std::sin(Pitch);
        return Normalize({cy*cp, sp, -sy*cp});
    }
    Vec3 Right() const { return Normalize(Cross(Forward(), {0,1,0})); }

    // Projection
    Mat4 Proj(float aspect) const {
        return Perspective(FovY, aspect, NearZ, FarZ);
    }
};
