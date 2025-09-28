// Copyright Lukas Licon 2025, All Rights Reservered.

#include "Madus/Renderer.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

static ShaderHandle GBasicShader=0;

static const char* VS = R"(#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNrm;
layout(location=2) in vec2 aUV;

uniform mat4 uModel, uView, uProj;
// NOTE: we still use mat3(uModel) — keep uniform scales for now
out vec3 vNrm; out vec3 vWS; out vec2 vUV;

void main(){
    vec4 ws = uModel * vec4(aPos,1.0);
    vWS = ws.xyz;
    vNrm = mat3(uModel) * aNrm;
    vUV = aUV;
    gl_Position = uProj * uView * ws;
})";

static const char* FS = R"(#version 330 core
in vec3 vNrm; in vec3 vWS; in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uAlbedo;
uniform vec3 uSunDir;        // light direction (toward light)
uniform vec3 uSunColor;
uniform float uSunIntensity;
uniform vec3 uCamPos;        // NEW
uniform vec3 uSkyColor;      // NEW (hemisphere top)
uniform vec3 uGroundColor;   // NEW (hemisphere bottom)

void main(){
    vec3 N = normalize(vNrm);
    vec3 L = normalize(-uSunDir);          // from point to light
    vec3 V = normalize(uCamPos - vWS);     // to camera
    vec3 H = normalize(L + V);

    float ndl = max(dot(N,L), 0.0);
    float ndh = max(dot(N,H), 0.0);
    float spec = pow(ndh, 32.0);           // simple Blinn-Phong

    // Hemisphere ambient
    float up = N.y * 0.5 + 0.5;
    vec3 hemi = mix(uGroundColor, uSkyColor, up);

    vec3 albedo = texture(uAlbedo, vUV).rgb;
    vec3 color = albedo * (hemi + uSunColor * (uSunIntensity * ndl))
               + 0.08 * spec;              // subtle specular

    FragColor = vec4(color, 1.0);
})";


void Renderer_Init(void*){
    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW); 

    GBasicShader = CreateShaderProgram(VS,FS);
}
void Renderer_Shutdown(){
    DestroyShaderProgram(GBasicShader); GBasicShader=0;
}
void Renderer_Resize(int w,int h){
    glViewport(0,0,w,h);
}
void Renderer_Begin(const FrameParams& fp){
    glClearColor(fp.Clear[0], fp.Clear[1], fp.Clear[2], 1.0f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glUseProgram(GBasicShader);
    // view/proj are set per-object OR could be set once here — we’ll set per-object for simplicity
    (void)fp;
}
void Renderer_DrawMesh(const GpuMesh& mesh, ShaderHandle sh, const Mat4& model, unsigned albedoTex){
    glUseProgram(sh);
    int locM = GetUniformLocation(sh,"uModel");
    int locV = GetUniformLocation(sh,"uView");
    int locP = GetUniformLocation(sh,"uProj");
    int locS = GetUniformLocation(sh,"uAlbedo");
    int locDir = GetUniformLocation(sh,"uSunDir");
    int locCol = GetUniformLocation(sh,"uSunColor");
    int locInt = GetUniformLocation(sh,"uSunIntensity");

    // Fetch from a tiny global cache — for simplicity store them static
    static Mat4 sView=Identity(), sProj=Identity();
    static DirectionalLight sSun{};
    // We rely on last set via Renderer_Begin? Expose setters — simplified here:
    // We hijack glGet uniforms? Too heavy. Instead, store in static during Begin:
    // For now, we let caller set uView/uProj before each batch; see Sandbox code below.

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, albedoTex);
    glUniform1i(locS, 0);

    // We need view/proj/sun each draw; we’ll set them via globals set by the Sandbox — to keep this file generic we expose a helper:
    // (In practice, set them right before DrawMesh from the caller)
    // No-op here; the caller will glUniformMatrix4fv for uView/uProj/dir/color/intensity right after calling this if needed.

    glUniformMatrix4fv(locM, 1, GL_FALSE, model.m);
    glBindVertexArray(mesh.vao);
    glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
void Renderer_End(){}

ShaderHandle Renderer_GetBasicLitShader(){ return GBasicShader; }
