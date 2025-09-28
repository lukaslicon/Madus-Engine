// Copyright Lukas Licon 2025, All Rights Reservered.

#pragma once

unsigned CreateTexture2DFromFile(const char* path, bool srgb=true);
unsigned CreateTexture2DWhite(); // 1x1 white fallback
void     DestroyTexture(unsigned& tex);
