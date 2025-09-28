// Copyright Lukas Licon 2025, All Rights Reservered.

#pragma once

#include "Madus/Math.h"
#include "Madus/Mesh.h"
#include "Madus/Shader.h"

struct DirectionalLight { float dir[3]={-0.3f,-1.f,-0.2f}; float color[3]={1,1,1}; float intensity=3.f; };

struct FrameParams {
    Mat4 View, Proj;
    DirectionalLight Sun;
    float Clear[3] = {0.06f, 0.07f, 0.09f};
};

void Renderer_Init(void* glfwWindow);
void Renderer_Shutdown();
void Renderer_Resize(int w,int h);
void Renderer_Begin(const FrameParams& fp);
void Renderer_DrawMesh(const GpuMesh& mesh, ShaderHandle sh, const Mat4& model, unsigned albedoTex);
void Renderer_End();
ShaderHandle Renderer_GetBasicLitShader();

// --- Shadow map API ---
struct ShadowMapInfo {
    Mat4 LightView;
    Mat4 LightProj;
    int  Size = 2048;
};

void Renderer_Shadow_Init(int size = 2048);
void Renderer_Shadow_Begin(const ShadowMapInfo& sm);
void Renderer_Shadow_DrawDepth(const GpuMesh& mesh, const Mat4& model);
void Renderer_Shadow_End();
unsigned Renderer_Shadow_GetTexture();
Mat4     Renderer_Shadow_GetLightVP();
