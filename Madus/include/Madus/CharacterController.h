// Copyright Lukas Licon 2025, All Rights Reservered.

#pragma once

#include "Madus/Math.h"
#include "Madus/Input.h"

struct CharacterController {
    Vec3  Position{0,1.2f,0};
    Vec3  Velocity{0,0,0};
    bool  Grounded = false;

    // Tunables
    float MoveSpeed    = 7.0f;
    float JumpSpeed    = 6.0f;
    float Gravity      = 22.0f;
    float DashSpeed    = 24.0f;
    float DashTime     = 0.12f;
    float DashCooldown = 0.35f;

    // Runtime
    float DashTimer    = 0.f;
    float DashCDTimer  = 0.f;

    void Tick(const InputState& in, float dt, const Vec3& camFwd, const Vec3& camRight);
};
