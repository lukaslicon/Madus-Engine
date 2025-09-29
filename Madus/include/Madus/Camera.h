// Copyright Lukas Licon 2025, All Rights Reserved.

#pragma once

#include "Madus/Math.h"
#include <algorithm>
#include <cmath>

enum class ProjectionType { Perspective, Ortho };

struct Camera {

    // projection 
    ProjectionType Type = ProjectionType::Perspective;
    float FovY   = 60.f * (float)MADUS_PI / 180.f; // radians
    float NearZ  = 0.05f;
    float FarZ   = 500.f;
    float OrthoH = 10.f; // half-height for Ortho

    // Placement / orientation
    Vec3  Pos{0, 1.7f, 6};
    float Yaw   = 0.f;   // radians, around +Y (world up)
    float Pitch = 0.f;   // radians, look up/down

    //  Directions 
    Vec3 Forward() const {
        float cy = std::cos(Yaw), sy = std::sin(Yaw);
        float cp = std::cos(Pitch), sp = std::sin(Pitch);
        return Normalize({cy*cp, sp, -sy*cp});
    }
    Vec3 Right() const { return Normalize(Cross(Forward(), {0,1,0})); }

    // Matrices
    Mat4 View() const {
        Vec3 fwd = Forward();
        return LookAt(Pos, Add(Pos, fwd), {0,1,0});
    }
    Mat4 Proj(float aspect) const {
        return (Type==ProjectionType::Perspective)
            ? Perspective(FovY, aspect, NearZ, FarZ)
            : Ortho(-OrthoH*aspect, OrthoH*aspect, -OrthoH, OrthoH, NearZ, FarZ);
    }

    // Controls 
    void AddYawPitch(float dYaw, float dPitch){
        Yaw += dYaw;
        Pitch = std::clamp(Pitch + dPitch,
            -(float)MADUS_PI/2.f + 0.001f,
            +(float)MADUS_PI/2.f - 0.001f);
    }
};
