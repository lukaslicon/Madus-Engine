// Copyright Lukas Licon 2025, All Rights Reserved.

#include "Madus/CharacterController.h"
#include <algorithm>

void CharacterController::Tick(const InputState& in, float dt, const Vec3& camFwd, const Vec3& camRight)
{
    // movement plane based on camera (XZ)
    Vec3 f = camFwd; f.y=0; f = Normalize(f);
    Vec3 r = camRight; r.y=0; r = Normalize(r);
    Vec3 wish = Add(Mul(f, in.MoveZ), Mul(r, in.MoveX));
    if (Length(wish)>1.f) wish = Normalize(wish);

    DashCDTimer = std::max(0.f, DashCDTimer - dt);

    if (DashTimer > 0.f){
        DashTimer -= dt; // maintain horizontal during dash
    } else {
        // regular horizontal
        Velocity.x = wish.x * MoveSpeed;
        Velocity.z = wish.z * MoveSpeed;

        // gravity
        Velocity.y -= Gravity * dt;

        // jump
        if (in.Jump && Grounded){
            Velocity.y = JumpSpeed;
            Grounded = false;
        }

        // start dash
        if (in.Dash && DashCDTimer<=0.f){
            Vec3 dir = (Length(wish)>0.1f) ? wish : f; // default forward
            Velocity.x = dir.x * DashSpeed;
            Velocity.z = dir.z * DashSpeed;
            DashTimer = DashTime;
            DashCDTimer = DashCooldown + DashTime;
        }
    }

    // temp ground plane at y=0
    Position = Add(Position, Mul(Velocity, dt));
    if (Position.y <= 0.f){
        Position.y = 0.f; Velocity.y = 0.f; Grounded = true;
    }
}
