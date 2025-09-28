// Copyright Lukas Licon 2025, All Rights Reservered.

#include "Madus/Renderer.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// Shaders
static ShaderHandle GBasicShader=0;

// Shadows
static unsigned gShadowTex = 0;
static unsigned gShadowFBO = 0;
static int      gShadowSize = 2048;
static ShaderHandle gShadowDepthShader = 0; // simple depth-only VS/FS
static Mat4     gLightVP = Identity();

//helper 
static Mat4 MulM(const Mat4& A, const Mat4& B){
    Mat4 R{};
    for(int c=0;c<4;++c)
        for(int r=0;r<4;++r)
            R.m[c*4+r] = A.m[0*4+r]*B.m[c*4+0] + A.m[1*4+r]*B.m[c*4+1] + A.m[2*4+r]*B.m[c*4+2] + A.m[3*4+r]*B.m[c*4+3];
    return R;
}

// Depth only shaders
static const char* VS_DEPTH = R"(#version 330 core
layout(location=0) in vec3 aPos;
uniform mat4 uModel;
uniform mat4 uLightView;
uniform mat4 uLightProj;
void main(){
    gl_Position = uLightProj * uLightView * uModel * vec4(aPos,1.0);
})";
static const char* FS_DEPTH = R"(#version 330 core
void main(){ /* depth only */ }
)";


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
uniform vec3  uSunDir;
uniform vec3  uSunColor;
uniform float uSunIntensity;
uniform vec3  uCamPos;
uniform vec3  uSkyColor;
uniform vec3  uGroundColor;

// Shadow
uniform sampler2D uShadowMap;
uniform mat4  uLightVP;

float ShadowFactor(vec3 ws){
    vec4 ls = uLightVP * vec4(ws,1.0);
    vec3 p = ls.xyz / ls.w;
    // to [0,1]
    vec2 uv = p.xy * 0.5 + 0.5;
    float z  = p.z * 0.5 + 0.5;

    // basic PCF 3x3
    float shadow = 0.0;
    float bias = 0.0015;
    vec2 texel = 1.0 / textureSize(uShadowMap, 0);
    for(int y=-1;y<=1;++y){
        for(int x=-1;x<=1;++x){
            float d = texture(uShadowMap, uv + vec2(x,y)*texel).r;
            shadow += (z - bias > d) ? 0.0 : 1.0;
        }
    }
    return shadow / 9.0; // 0..1 (0=full shadow, 1=lit)
}

void main(){
    vec3 N = normalize(vNrm);
    vec3 L = normalize(-uSunDir);
    vec3 V = normalize(uCamPos - vWS);
    vec3 H = normalize(L + V);

    float ndl = max(dot(N,L), 0.0);
    float ndh = max(dot(N,H), 0.0);
    float spec = pow(ndh, 32.0);

    float up = N.y * 0.5 + 0.5;
    vec3 hemi = mix(uGroundColor, uSkyColor, up);

    float vis = ShadowFactor(vWS);

    vec3 albedo = texture(uAlbedo, vUV).rgb;
    vec3 color = albedo * (hemi + vis * (uSunColor * (uSunIntensity * ndl)))
               + 0.08 * spec * vis;

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


void Renderer_Shadow_Init(int size){
    gShadowSize = size;

    glGenTextures(1, &gShadowTex);
    glBindTexture(GL_TEXTURE_2D, gShadowTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, gShadowSize, gShadowSize, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float border[4] = {1,1,1,1};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);

    glGenFramebuffers(1, &gShadowFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, gShadowFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gShadowTex, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    gShadowDepthShader = CreateShaderProgram(VS_DEPTH, FS_DEPTH);
}

void Renderer_Shadow_Begin(const ShadowMapInfo& sm){
    gLightVP = MulM(sm.LightProj, sm.LightView); // store LightVP for main pass

    glViewport(0,0,gShadowSize,gShadowSize);
    glBindFramebuffer(GL_FRAMEBUFFER, gShadowFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.0f, 4.0f);
    glCullFace(GL_FRONT);

    glUseProgram(gShadowDepthShader);
    int locLV = GetUniformLocation(gShadowDepthShader, "uLightView");
    int locLP = GetUniformLocation(gShadowDepthShader, "uLightProj");
    glUniformMatrix4fv(locLV, 1, GL_FALSE, sm.LightView.m);
    glUniformMatrix4fv(locLP, 1, GL_FALSE, sm.LightProj.m);
}

void Renderer_Shadow_DrawDepth(const GpuMesh& mesh, const Mat4& model){
    int locM = GetUniformLocation(gShadowDepthShader, "uModel");
    glUniformMatrix4fv(locM, 1, GL_FALSE, model.m);
    glBindVertexArray(mesh.vao);
    glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
void Renderer_Shadow_End(){
    glCullFace(GL_BACK);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
unsigned Renderer_Shadow_GetTexture(){ return gShadowTex; }
Mat4 Renderer_Shadow_GetLightVP(){ return gLightVP; } // caller will upload (we’ll compute properly in Sandbox)

