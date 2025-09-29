// Copyright Lukas Licon 2025, All Rights Reserved.

#include "Madus/Renderer.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// Shaders
static ShaderHandle GBasicShader = 0;
static ShaderHandle GSkyShader = 0;
static GLuint gDummyVAO = 0;

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

// Depth shaders
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

    // --- 5x5 PCF ---
    float shadow = 0.0;
    float bias = 0.0015;                       // tweak 0.0008 .. 0.003
    vec2 texel = 1.0 / textureSize(uShadowMap, 0);

    for (int y = -2; y <= 2; ++y) {
        for (int x = -2; x <= 2; ++x) {
            float d = texture(uShadowMap, uv + vec2(x,y) * texel).r;
            shadow += (z - bias > d) ? 0.0 : 1.0;
        }
    }
    return shadow / 25.0; // 0..1 (0=full shadow, 1=lit)
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

static const char* VS_SKY = R"(#version 330 core
const vec2 verts[3] = vec2[3]( vec2(-1.0,-1.0), vec2(3.0,-1.0), vec2(-1.0,3.0) );
out vec2 vNDC;
void main(){
    vNDC = verts[gl_VertexID];         // NDC in [-1,1] (will use to build ray dir)
    gl_Position = vec4(verts[gl_VertexID], 0.0, 1.0);
})";

static const char* FS_SKY = R"(#version 330 core
in vec2 vNDC;
out vec4 FragColor;

uniform mat4 uView;
uniform mat4 uProj;
uniform vec3 uSunDir;        // normalized, same one you use for lighting/shadows
uniform vec3 uSkyColor;
uniform vec3 uGroundColor;
uniform float uSunSizeDeg;   // try 1.5 first to verify visibility
uniform float uSunIntensity; // try 7.0 first

// Robust: use inverse(projection) instead of fx/fy
vec3 RayDirWorld(vec2 ndc){
    // reconstruct a point on the view frustum at z=1 in clip space
    vec4 clip = vec4(ndc, 1.0, 1.0);
    // go to view space
    vec4 viewP = inverse(uProj) * clip;
    vec3 dirV = normalize(viewP.xyz / viewP.w);
    // remove camera rotation to world
    mat3 Rinv = transpose(mat3(uView));
    return normalize(Rinv * dirV);
}

void main(){
    vec2 ndc = vNDC; // interpolated from the full-screen triangle [-1..1]
    vec3 d = RayDirWorld(ndc);

    // Hemisphere gradient
    float t = d.y * 0.5 + 0.5;
    vec3 base = mix(uGroundColor, uSkyColor, t);

    // --- Sun disk ---
    // If you still don't see it, try flipping this sign once:
    // vec3 sunLook = normalize(+uSunDir); // (test flip)
    vec3 sunLook = normalize(-uSunDir);     // light comes from the sun toward the scene
    float sd = clamp(dot(d, sunLook), 0.0, 1.0);

    float r  = radians(uSunSizeDeg);         // hard radius, in degrees
    float rs = r * 1.5;                      // soft edge
    float disk  = smoothstep(cos(rs), cos(r), sd);
    float halo  = smoothstep(0.92, 1.0, sd) * 0.4;

    vec3 col = base + (disk * uSunIntensity) + (halo * uSunIntensity * 0.35);
    FragColor = vec4(col, 1.0);
})";


void Renderer_Init(void*){
    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    GBasicShader = CreateShaderProgram(VS,FS);
    GSkyShader   = CreateShaderProgram(VS_SKY, FS_SKY);

    glGenVertexArrays(1, &gDummyVAO);
    glBindVertexArray(gDummyVAO); 
}

void Renderer_Shutdown(){
    DestroyShaderProgram(GBasicShader); GBasicShader = 0;
    DestroyShaderProgram(GSkyShader);   GSkyShader   = 0; 
}
void Renderer_Resize(int w,int h){
    glViewport(0,0,w,h);
}
void Renderer_Begin(const FrameParams& fp){
    glClearColor(fp.Clear[0], fp.Clear[1], fp.Clear[2], 1.0f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glUseProgram(GBasicShader);
    (void)fp;
}
void Renderer_DrawMesh(const GpuMesh& mesh, ShaderHandle sh, const Mat4& model, unsigned albedoTex){
    glUseProgram(sh);
    int locM = GetUniformLocation(sh,"uModel");
    int locS = GetUniformLocation(sh,"uAlbedo");

    static Mat4 sView=Identity(), sProj=Identity();
    static DirectionalLight sSun{};

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, albedoTex);
    glUniform1i(locS, 0);

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
    gLightVP = MulM(sm.LightProj, sm.LightView); 

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
Mat4 Renderer_Shadow_GetLightVP(){ return gLightVP; } 

void Renderer_DrawSky(const Mat4& view, const Mat4& proj, const DirectionalLight& sun){
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    glUseProgram(GSkyShader);
    glUniformMatrix4fv(GetUniformLocation(GSkyShader,"uView"),1,GL_FALSE,view.m);
    glUniformMatrix4fv(GetUniformLocation(GSkyShader,"uProj"),1,GL_FALSE,proj.m);
    glUniform3f(GetUniformLocation(GSkyShader,"uSunDir"), sun.dir[0], sun.dir[1], sun.dir[2]);
    glUniform3f(GetUniformLocation(GSkyShader,"uSkyColor"), 0.32f,0.42f,0.62f);
    glUniform3f(GetUniformLocation(GSkyShader,"uGroundColor"), 0.10f,0.09f,0.09f);

    glUniform1f(GetUniformLocation(GSkyShader,"uSunSizeDeg"), 0.6f);  
    glUniform1f(GetUniformLocation(GSkyShader,"uSunIntensity"), 1.0f); 



    extern GLuint gDummyVAO; 
    glBindVertexArray(gDummyVAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glEnable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}

