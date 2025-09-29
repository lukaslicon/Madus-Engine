// Copyright Lukas Licon 2025, All Rights Reserved.

#pragma once

#include "Madus/Math.h"
#include "Madus/Input.h"

// Basic locomotion states
enum class EPlayerState { Idle, Move, Jump, Fall, Dash };

struct CharacterController {
    // Pose
    Vec3  Position{0, 1.2f, 0};  
    Vec3  Velocity{0, 0, 0};
    bool  Grounded = false;

    // Movement
    float MaxSpeedGround = 7.5f;
    float MaxSpeedAir    = 7.5f;
    float AccelGround    = 38.0f;
    float AccelAir       = 12.0f;
    float Friction       = 10.0f;

    // Jumping
    float JumpSpeed      = 6.5f;
    float Gravity        = 22.0f;
    float CoyoteTime     = 0.10f;
    float BufferWindow   = 0.15f;

    // Dashing
    float DashSpeed      = 24.0f;
    float DashTime       = 0.14f;   
    float DashCooldown   = 0.35f; 
    float DashIFrameBeg  = 0.0f;    
    float DashIFrameEnd  = 0.80f;   

    // Braking 
    float BrakeDecel        = 20.0f;  
    float StopSpeedEpsilon  = 0.06f;  

    // Capsule & ground
    float CapsuleRadius     = 0.35f;
    float CapsuleHalfHeight = 0.90f;
    float GroundY           = 0.0f; 

    // Walking constraints
    float StepOffset   = 0.40f;     
    float MaxSlopeDeg  = 40.0f;      
    float GroundSnap   = 0.02f;      


    EPlayerState State = EPlayerState::Idle;
    float OnGroundTime = 0.f;    
    float OffGroundTime = 0.f;   

    float DashTimer   = 0.f;
    float DashCDTimer = 0.f;
    bool  Invulnerable = false;

    // Input buffering
    float JumpBuf = 0.f;
    float DashBuf = 0.f;

    // For debug overlay
    float LastSpeed = 0.f;   
    float AccelMag  = 0.f;   

    void Tick(const InputState& in, float dt, const Vec3& camFwd, const Vec3& camRight);

    using GroundHeightFn = float(*)(float x, float z);
    using GroundNormalFn = Vec3 (*)(float x, float z);

    GroundHeightFn GroundHeight = nullptr; // if null, returns GroundY
    GroundNormalFn GroundNormal = nullptr; // if null, returns {0,1,0}
};
