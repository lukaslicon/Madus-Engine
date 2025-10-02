// Copyright Lukas Licon 2025, All Rights Reserved.

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <algorithm> // std::clamp, std::min/max
#include <vector>
#include <fstream>
#include <sstream>
#include <string>

#include "Madus/Math.h"
#include "Madus/Camera.h"
#include "Madus/Input.h"
#include "Madus/Mesh.h"
#include "Madus/Texture.h"
#include "Madus/Renderer.h"
#include "Madus/CharacterController.h"

static void GLAPIENTRY glDbg(GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar* msg, const void*) {
    std::cerr << "[GL] " << msg << "\n";
}

static Mat4 MulM(const Mat4& A, const Mat4& B){
    Mat4 R{};
    for(int c=0;c<4;++c)
        for(int r=0;r<4;++r)
            R.m[c*4+r] = A.m[0*4+r]*B.m[c*4+0] + A.m[1*4+r]*B.m[c*4+1]
                       + A.m[2*4+r]*B.m[c*4+2] + A.m[3*4+r]*B.m[c*4+3];
    return R;
}

static Vec3 LerpExp(const Vec3& from, const Vec3& to, float dt, float halfLifeSeconds)
{
    if (halfLifeSeconds <= 0.f) return to;
    const float lambda = std::log(2.f) / halfLifeSeconds;
    const float t = 1.f - std::exp(-lambda * dt);
    return Add(from, Mul(Add(to, Mul(from, -1.f)), t));
}

// ----------------------------
// Simple 2D AABB on XZ plane
// ----------------------------
struct AABB2 { float minx, minz, maxx, maxz; };

// ----------------------------
// Level loader (text file)
// Each non-empty line: minx minz maxx maxz
// '#' or '//' start a comment
// ----------------------------
struct Level {
    std::vector<AABB2> Colliders;

    static bool IsCommentOrBlank(const std::string& s){
        size_t i = s.find_first_not_of(" \t\r\n");
        if (i == std::string::npos) return true;
        if (s[i] == '#') return true;
        if (i+1 < s.size() && s[i]=='/' && s[i+1]=='/') return true;
        return false;
    }

    bool LoadTxt(const char* path){
        std::ifstream f(path);
        if (!f.is_open()){
            std::printf("[Level] Failed to open '%s'\n", path);
            return false;
        }
        Colliders.clear();
        std::string line;
        int lineno = 0, added = 0;
        while (std::getline(f, line)){
            ++lineno;
            if (IsCommentOrBlank(line)) continue;
            std::istringstream iss(line);
            float minx, minz, maxx, maxz;
            if (!(iss >> minx >> minz >> maxx >> maxz)){
                std::printf("[Level] Parse error at %s:%d -> '%s'\n", path, lineno, line.c_str());
                continue;
            }
            if (maxx < minx) std::swap(maxx, minx);
            if (maxz < minz) std::swap(maxz, minz);
            Colliders.push_back({minx, minz, maxx, maxz});
            ++added;
        }
        std::printf("[Level] Loaded %d colliders from '%s'\n", added, path);
        return (added > 0);
    }
};

// ------------------------------------------------------
// Circle (hero) vs AABB2 resolve on XZ, keeps on ground
// (This stays identical to your old resolver)
// ------------------------------------------------------
static bool ResolveCircleAABB2(Vec3& pos, Vec3& vel, float radius, const AABB2& b)
{
    float qx = std::min(std::max(pos.x, b.minx), b.maxx);
    float qz = std::min(std::max(pos.z, b.minz), b.maxz);
    float dx = pos.x - qx;
    float dz = pos.z - qz;
    float d2 = dx*dx + dz*dz;

    if (d2 > 0.0f) {
        float r = radius;
        if (d2 < r*r) {
            float d = std::sqrt(d2);
            float nx = dx / d, nz = dz / d;
            float push = (r - d);
            pos.x += nx * push; pos.z += nz * push;
            float vn = vel.x*nx + vel.z*nz;
            if (vn < 0.f) { vel.x -= vn*nx; vel.z -= vn*nz; }
            return true;
        }
        return false;
    } else {
        float left   = pos.x - b.minx;
        float right  = b.maxx - pos.x;
        float down   = pos.z - b.minz;
        float up     = b.maxz - pos.z;
        float minX = std::min(left, right);
        float minZ = std::min(down, up);
        if (minX < minZ) {
            float nx = (left < right) ? 1.f : -1.f;
            float push = minX + radius;
            pos.x += nx * push;
            float vn = vel.x*nx;
            if (vn < 0.f) vel.x -= vn*nx;
        } else {
            float nz = (down < up) ? 1.f : -1.f;
            float push = minZ + radius;
            pos.z += nz * push;
            float vn = vel.z*nz;
            if (vn < 0.f) vel.z -= vn*nz;
        }
        return true;
    }
}

int main(){
    if(!glfwInit()){ std::cerr<<"GLFW init failed\n"; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* win = glfwCreateWindow(1920,1080,"Madus Sandbox",nullptr,nullptr);
    if(!win){ glfwTerminate(); return -1; }
    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);
    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwRawMouseMotionSupported()) glfwSetInputMode(win, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    bool gMouseCaptured = true;

    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){ std::cerr<<"glad failed\n"; return -1; }
#ifndef NDEBUG
    if (glDebugMessageCallback) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(glDbg, nullptr);
    }
#endif

    Input_BindWindow(win);
    Renderer_Init(win);
    Renderer_Shadow_Init(2048);

    int w=1920,h=1080;
    Renderer_Resize(w,h);
    Camera cam;

    auto DegToRad = [](float d){ return d * (float)MADUS_PI / 180.f; };

    const float baseYawDeg   = 45.f;    // face down world diagonal
    const float basePitchDeg = -35.f;   // tilt downward

    cam.FovY  = DegToRad(65.f);
    cam.Pos   = {0, 8, 12}; // gets snapped below anyway

    // Boom parameters (constant)
    const float camBack   = 12.0f;
    const float camUp     = 8.0f;
    const float eyeLift   = 1.0f;

    const float maxHoriz = 2.0f;
    const float maxVert  = 1.5f;
    const float panSpeed = 3.0f;
    const float springHalfLife = 0.25f;

    float targetOffX = 0.f;
    float targetOffY = 0.f;

    // Geometry & materials
    GpuMesh plane = CreatePlane(40.f);
    GpuMesh box   = CreateBoxUnit();
    unsigned ground = CreateCheckerTexture(1024, 16, true);
    unsigned white  = CreateTexture2DWhite();

    ShaderHandle sh = Renderer_GetBasicLitShader();

    CharacterController hero{};
    hero.Position = {0, 0, 0};

    double lastTime = glfwGetTime();

    // --------- Level: load from file; fallback to your old hardcoded layout ---------
    Level level;
    if (!level.LoadTxt("assets/levels/room01.txt")) {
        std::printf("[Level] Using fallback layout\n");
        // Level bounds: a 20x20 playable square, walls 1m thick around it
        const float halfW = 19.0f, halfD = 19.0f, th = 1.0f;
        level.Colliders.push_back({-halfW-th, -halfD-th, -halfW,   halfD+th}); // left wall
        level.Colliders.push_back({ halfW,    -halfD-th,  halfW+th, halfD+th}); // right wall
        level.Colliders.push_back({-halfW,    -halfD-th,  halfW,   -halfD});    // bottom wall
        level.Colliders.push_back({-halfW,     halfD,     halfW,    halfD+th}); // top wall
        // A pillar near the center (1.2m square)
        level.Colliders.push_back({-0.6f, -0.6f, +0.6f, +0.6f});
    }

    while(!glfwWindowShouldClose(win)){
        glfwPollEvents();

        int fbw, fbh; glfwGetFramebufferSize(win, &fbw, &fbh);
        if (fbw!=w || fbh!=h){ w=fbw; h=fbh; Renderer_Resize(w,h); }

        double now = glfwGetTime();
        float dt = float(now - lastTime);
        lastTime = now;

        InputState in{}; Input_Poll(in);

        // Press ESC to release cursor
        if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS && gMouseCaptured) {
            glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            if (glfwRawMouseMotionSupported()) glfwSetInputMode(win, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
            gMouseCaptured = false;
            Input_SetActive(false);
            Input_ResetMouse();
        }
        // Press LMB to recapture cursor
        if (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && !gMouseCaptured) {
            glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            if (glfwRawMouseMotionSupported()) glfwSetInputMode(win, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
            gMouseCaptured = true;
            Input_SetActive(true);
            Input_ResetMouse();
        }

        if (Input_IsActive()) {
            hero.Tick(in, dt, cam.Forward(), cam.Right());
        }

        // Collide hero with level colliders on XZ
        for (const AABB2& b : level.Colliders) {
            ResolveCircleAABB2(hero.Position, hero.Velocity, hero.CapsuleRadius, b);
        }

        // HUD title (unchanged)
        static float hudAccum = 0.f;
        hudAccum += dt;
        if (hudAccum > 0.10f) {
            hudAccum = 0.f;
            const char* stateStr = "Idle";
            switch (hero.State) {
                case EPlayerState::Idle: stateStr = "Idle"; break;
                case EPlayerState::Move: stateStr = "Move"; break;
                case EPlayerState::Jump: stateStr = "Jump"; break;
                case EPlayerState::Fall: stateStr = "Fall"; break;
                case EPlayerState::Dash: stateStr = "Dash"; break;
            }
            char title[256];
            std::snprintf(title, sizeof(title),
                "Madus Sandbox | spd=%.2f m/s  acc=%.1f m/s^2  state=%s  dashT=%.2f cd=%.2f  invul=%s  grounded=%s",
                hero.LastSpeed, hero.AccelMag, stateStr, hero.DashTimer, hero.DashCDTimer,
                hero.Invulnerable ? "Y" : "N",
                hero.Grounded ? "Y" : "N");
            glfwSetWindowTitle(win, title);
        }

        const float baseYaw   = DegToRad(baseYawDeg);
        const float basePitch = DegToRad(basePitchDeg);

        auto ForwardFrom = [](float yaw, float pitch)->Vec3 {
            float cy = std::cos(yaw), sy = std::sin(yaw);
            float cp = std::cos(pitch), sp = std::sin(pitch);
            return Normalize(Vec3{ cy*cp, sp, -sy*cp });
        };

        Vec3 baseFwd   = ForwardFrom(baseYaw, basePitch);
        Vec3 baseRight = Normalize(Cross(baseFwd, Vec3{0,1,0}));
        cam.Pos   = Add(hero.Position, Add(Mul(baseFwd, -12.0f), Vec3{0, 8.0f, 0}));
        cam.Yaw   = baseYaw;
        cam.Pitch = basePitch;

        if (glfwGetKey(win, GLFW_KEY_RIGHT) == GLFW_PRESS) targetOffX += 3.0f * dt;
        if (glfwGetKey(win, GLFW_KEY_LEFT)  == GLFW_PRESS) targetOffX -= 3.0f * dt;
        if (glfwGetKey(win, GLFW_KEY_UP)    == GLFW_PRESS) targetOffY += 3.0f * dt;
        if (glfwGetKey(win, GLFW_KEY_DOWN)  == GLFW_PRESS) targetOffY -= 3.0f * dt;

        targetOffX = std::clamp(targetOffX, -2.0f, +2.0f);
        targetOffY = std::clamp(targetOffY, -1.5f, +1.5f);

        auto Spring01 = [](float v, float dt, float halfLife){
            if (halfLife <= 0.f) return v;
            float k = 1.f - std::exp(-std::log(2.f) * dt / halfLife);
            return v * (1.f - k);
        };
        bool anyH = (glfwGetKey(win, GLFW_KEY_LEFT)==GLFW_PRESS) || (glfwGetKey(win, GLFW_KEY_RIGHT)==GLFW_PRESS);
        bool anyV = (glfwGetKey(win, GLFW_KEY_UP)==GLFW_PRESS)   || (glfwGetKey(win, GLFW_KEY_DOWN)==GLFW_PRESS);
        if (!anyH) targetOffX = Spring01(targetOffX, dt, 0.25f);
        if (!anyV) targetOffY = Spring01(targetOffY, dt, 0.25f);

        Vec3 target = Add(hero.Position, Vec3{0, 1.0f, 0});
        target = Add(target, Mul(baseRight, targetOffX));
        target = Add(target, Mul(Vec3{0,1,0}, targetOffY));

        FrameParams fp{};
        fp.View = LookAt(cam.Pos, target, Vec3{0,1,0});
        fp.Proj = cam.Proj((float)w/(float)h);
        fp.Sun  = DirectionalLight{};
        fp.Sun.dir[0] = -0.35f; fp.Sun.dir[1] = -0.90f; fp.Sun.dir[2] = -0.20f;
        fp.Sun.intensity = 3.0f;

        Vec3 sunDir = Normalize(Vec3{ fp.Sun.dir[0], fp.Sun.dir[1], fp.Sun.dir[2] });
        fp.Sun.dir[0] = sunDir.x;
        fp.Sun.dir[1] = sunDir.y;
        fp.Sun.dir[2] = sunDir.z;

        // ------------- SHADOW PASS -------------
        Vec3 center = hero.Position; center.y = 0.0f;
        float lightDist = 30.0f;
        Vec3 lightPos = Add(center, Mul(sunDir, -lightDist));  // center - dir * dist

        Mat4 LView = LookAt(lightPos, center, {0,1,0});
        float R = 18.0f;
        Mat4 LProj = Ortho(-R, R, -R, R, 0.1f, 80.0f);

        ShadowMapInfo sm{ LView, LProj, 2048 };
        Renderer_Shadow_Begin(sm);
        {
            Renderer_Shadow_DrawDepth(plane, TRS({0,0,0}, AngleAxis(0,{0,1,0}), {1,1,1}));
            Renderer_Shadow_DrawDepth(box,   TRS(hero.Position, AngleAxis(0,{0,1,0}), {1,1,1}));
            // Level walls into shadow map
            for (const AABB2& b : level.Colliders) {
                float cx = 0.5f*(b.minx + b.maxx);
                float cz = 0.5f*(b.minz + b.maxz);
                float sx = (b.maxx - b.minx);
                float sz = (b.maxz - b.minz);
                Mat4 M = TRS(Vec3{cx, 1.0f, cz}, AngleAxis(0,{0,1,0}), Vec3{sx, 3.0f, sz});
                Renderer_Shadow_DrawDepth(box, M);
            }
        }
        Renderer_Shadow_End();

        // Restore viewport after shadow pass
        glViewport(0,0,w,h);

        // -------------- MAIN PASS --------------
        Renderer_Begin(fp);

        // sky
        Renderer_DrawSky(fp.View, fp.Proj, fp.Sun);

        glUseProgram(sh);

        // Common uniforms
        int locV   = GetUniformLocation(sh,"uView");
        int locP   = GetUniformLocation(sh,"uProj");
        int locDir = GetUniformLocation(sh,"uSunDir");
        int locCol = GetUniformLocation(sh,"uSunColor");
        int locInt = GetUniformLocation(sh,"uSunIntensity");
        int locCam = GetUniformLocation(sh,"uCamPos");
        int locSky = GetUniformLocation(sh,"uSkyColor");
        int locGnd = GetUniformLocation(sh,"uGroundColor");
        int locSh  = GetUniformLocation(sh,"uShadowMap");
        int locLVP = GetUniformLocation(sh,"uLightVP");

        // upload view/proj
        glUniformMatrix4fv(locV,1,GL_FALSE, fp.View.m);
        glUniformMatrix4fv(locP,1,GL_FALSE, fp.Proj.m);

        // upload sun using SAME normalized vector
        glUniform3f(locDir, sunDir.x, sunDir.y, sunDir.z);
        glUniform3f(locCol, fp.Sun.color[0], fp.Sun.color[1], fp.Sun.color[2]);
        glUniform1f(locInt, fp.Sun.intensity);

        // camera + hemisphere colors
        glUniform3f(locCam, cam.Pos.x, cam.Pos.y, cam.Pos.z);
        glUniform3f(locSky, 0.32f, 0.42f, 0.62f);
        glUniform3f(locGnd, 0.10f, 0.09f, 0.09f);

        // shadow bindings
        Mat4 LightVP = MulM(LProj, LView); // order: P * V
        glUniformMatrix4fv(locLVP, 1, GL_FALSE, LightVP.m);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, Renderer_Shadow_GetTexture());
        glUniform1i(locSh, 1);
        glActiveTexture(GL_TEXTURE0);

        // draw ground
        Mat4 Mground = TRS({0,0,0}, AngleAxis(0,{0,1,0}), {1,1,1});
        Renderer_DrawMesh(plane, sh, Mground, ground);

        // hero proxy
        Mat4 Mhero = TRS(hero.Position, AngleAxis(0,{0,1,0}), {1.0f,1.5f,1.0f});
        Renderer_DrawMesh(box, sh, Mhero, white);

        // collider visualization (from level.Colliders)
        for (const AABB2& b : level.Colliders) {
            float cx = 0.5f*(b.minx + b.maxx);
            float cz = 0.5f*(b.minz + b.maxz);
            float sx = (b.maxx - b.minx);
            float sz = (b.maxz - b.minz);
            // make them 3m tall so they're visible
            Mat4 M = TRS(Vec3{cx, 1.0f, cz}, AngleAxis(0,{0,1,0}), Vec3{sx, 3.0f, sz});
            Renderer_DrawMesh(box, sh, M, white);
        }

        Renderer_End();
        glfwSwapBuffers(win);

        in.ClearFrameDeltas();
    }

    DestroyTexture(ground);
    DestroyTexture(white);
    DestroyMesh(box);
    DestroyMesh(plane);
    Renderer_Shutdown();
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
