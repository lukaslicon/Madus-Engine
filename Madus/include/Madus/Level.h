// Copyright Lukas Licon 2025, All Rights Reserved.

#pragma once

#include <vector>
#include "Madus/Math.h" // for AABB2

struct Level {
    std::vector<AABB2> Colliders;

    bool LoadTxt(const char* path);
};