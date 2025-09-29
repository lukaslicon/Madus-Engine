// Copyright Lukas Licon 2025, All Rights Reservered.

#pragma once

#include <string>
using ShaderHandle = unsigned;
ShaderHandle CreateShaderProgram(const char* vsSrc, const char* fsSrc);
void         DestroyShaderProgram(ShaderHandle);
int          GetUniformLocation(ShaderHandle, const char* name);
