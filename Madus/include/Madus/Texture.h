// Copyright Lukas Licon 2025, All Rights Reservered.

#pragma once

unsigned CreateTexture2DFromFile(const char* path, bool srgb=true);
unsigned CreateTexture2DWhite(); // 1x1 white fallback
void     DestroyTexture(unsigned& tex);
unsigned CreateCheckerTexture(int size = 1024, int checks = 16, bool srgb = true);

