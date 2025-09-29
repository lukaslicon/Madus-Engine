// Copyright Lukas Licon 2025, All Rights Reserved.

#include "Madus/CharacterController.h"
#include <algorithm>
#include <cmath>

static inline Vec3 Flatten(const Vec3& v){ return Vec3{v.x, 0.f, v.z}; }
static inline float Dot2D(const Vec3& a, const Vec3& b){ return a.x*b.x + a.z*b.z; }
static inline float Len2D(const Vec3& v){float s = v.x * v.x + v.z * v.z; return (s > 0.f) ? std::sqrt(s) : 0.f;}
static inline float Clamp(float x, float a, float b){ return std::max(a, std::min(b, x)); }



static void AccelerateXZ(Vec3& vel, const Vec3& wishDir, float targetSpeed, float accel, float dt)
{
    float curSpeedAlong = Dot2D(vel, wishDir);
    float add = targetSpeed - curSpeedAlong;
    if (add <= 0.f) return;
    float accelStep = accel * dt;
    if (accelStep > add) accelStep = add;
    vel.x += wishDir.x * accelStep;
    vel.z += wishDir.z * accelStep;
}

static void ApplyFrictionXZ(Vec3& vel, float friction, float dt)
{
    float speed = std::sqrt(vel.x*vel.x + vel.z*vel.z);
    if (speed <= 1e-4f) { vel.x = vel.z = 0.f; return; }
    float drop = friction * dt;
    float newSpeed = std::max(0.f, speed - drop);
    if (newSpeed != speed){
        float scale = newSpeed / speed;
        vel.x *= scale; vel.z *= scale;
    }
}

static void ApplyBrakingXZ(Vec3& vel, float decel, float dt, float stopEps)
{
    float speed = std::sqrt(vel.x*vel.x + vel.z*vel.z);
    if (speed <= stopEps) { vel.x = 0.f; vel.z = 0.f; return; }
    float newSpeed = std::max(0.f, speed - decel * dt);
    float scale = (speed > 0.f) ? (newSpeed / speed) : 0.f;
    vel.x *= scale; vel.z *= scale;
    if (newSpeed <= stopEps) { vel.x = 0.f; vel.z = 0.f; }
}


void CharacterController::Tick(const InputState& in, float dt, const Vec3& camFwd, const Vec3& camRight)
{
    if (in.Jump) JumpBuf = BufferWindow;
    if (in.Dash) DashBuf = BufferWindow;

    Vec3 f = camFwd; f.y = 0; if (Length(f) > 0.0001f) f = Normalize(f);
    Vec3 r = camRight; r.y = 0; if (Length(r) > 0.0001f) r = Normalize(r);
    Vec3 wish = Add(Mul(f, in.MoveZ), Mul(r, in.MoveX));
    float wishLen = Len2D(wish);
    if (wishLen > 1.f) {
        wish = Mul(wish, 1.f / wishLen);
    }

    DashCDTimer = std::max(0.f, DashCDTimer - dt);
    JumpBuf     = std::max(0.f, JumpBuf     - dt);
    DashBuf     = std::max(0.f, DashBuf     - dt);

    float capsuleBottomY = Position.y - CapsuleHalfHeight;
    Grounded = (capsuleBottomY <= GroundY + 0.0001f) && (Velocity.y <= 0.0f);

    if (Grounded){
        OnGroundTime += dt; OffGroundTime = 0.f;
        float desiredY = GroundY + CapsuleHalfHeight;
        if (Position.y != desiredY){ Position.y = desiredY; if (Velocity.y < 0.f) Velocity.y = 0.f; }
    } else {
        OffGroundTime += dt; OnGroundTime = 0.f;
    }

    if (DashTimer > 0.f){
        DashTimer -= dt;
        Velocity.y -= Gravity * dt * 0.25f; 
        float frac = 1.f - (DashTimer / DashTime);
        Invulnerable = (frac >= DashIFrameBeg && frac <= DashIFrameEnd);

        if (DashTimer <= 0.f){
            Invulnerable = false;
            const float damp = 0.35f;
            Velocity.x *= damp;
            Velocity.z *= damp;

            State = Grounded ? EPlayerState::Idle : EPlayerState::Fall;
        } else {
            State = EPlayerState::Dash;
        }
    }
    else
    {
        if (DashBuf > 0.f && DashCDTimer <= 0.f){
            Vec3 dir = (wishLen > 0.1f) ? wish : f;
            if (Length(dir) < 1e-4f) dir = Vec3{1,0,0}; 
            dir = Normalize(dir);
            Velocity.x = dir.x * DashSpeed;
            Velocity.z = dir.z * DashSpeed;
            DashTimer  = DashTime;
            DashCDTimer = DashCooldown + DashTime; 
            Invulnerable = true; 
            State = EPlayerState::Dash;
            DashBuf = 0.f; 
        }
        else
        {
            Velocity.y -= Gravity * dt;
            bool canJump = (Grounded || OffGroundTime <= CoyoteTime);
            if (JumpBuf > 0.f && canJump){
                Velocity.y = JumpSpeed;
                Grounded = false;
                OffGroundTime = 0.f;
                State = EPlayerState::Jump;
                JumpBuf = 0.f; 
            }

            float targetMax = Grounded ? MaxSpeedGround : MaxSpeedAir;
            float accel     = Grounded ? AccelGround    : AccelAir;

            if (wishLen > 0.001f){
                AccelerateXZ(Velocity, wish, targetMax, accel, dt);
                float sp = std::sqrt(Velocity.x*Velocity.x + Velocity.z*Velocity.z);
                if (sp > targetMax){
                    float s = targetMax / sp;
                    Velocity.x *= s; Velocity.z *= s;
                }
                State = Grounded ? EPlayerState::Move : (Velocity.y > 0.f ? EPlayerState::Jump : EPlayerState::Fall);
            } else {
                if (Grounded) {
                    ApplyBrakingXZ(Velocity, BrakeDecel, dt, StopSpeedEpsilon);
                } else {
                    //  tiny air drag for air glide:
                     ApplyFrictionXZ(Velocity, Friction * 0.2f, dt);
                }
                State = Grounded ? EPlayerState::Idle : (Velocity.y > 0.f ? EPlayerState::Jump : EPlayerState::Fall);
            }

        }
    }

    Vec3 prevVel = Velocity;
    Position = Add(Position, Mul(Velocity, dt));

    float floorY = GroundY + CapsuleHalfHeight;
    if (Position.y < floorY){
        Position.y = floorY;
        if (Velocity.y < 0.f) Velocity.y = 0.f;
        Grounded = true;
        OffGroundTime = 0.f;
    }

    float curSpeed = std::sqrt(Velocity.x*Velocity.x + Velocity.z*Velocity.z);
    AccelMag = (dt > 0.f) ? (curSpeed - LastSpeed) / dt : 0.f;
    LastSpeed = curSpeed;
}
