// Copyright Lukas Licon 2025, All Rights Reserved.

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <algorithm> // std::clamp

#include "Madus/Math.h"
#include "Madus/Camera.h"
#include "Madus/Input.h"
#include "Madus/Mesh.h"
#include "Madus/Texture.h"
#include "Madus/Renderer.h"
#include "Madus/CharacterController.h"

// -----------------------------------------------
// GL debug callback
// -----------------------------------------------
static void GLAPIENTRY glDbg(GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar* msg, const void*) {
    std::cerr << "[GL] " << msg << "\n";
}

// -----------------------------------------------
// Tiny Mat4 multiply helper (column-major)
// -----------------------------------------------
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

int main(){
    if(!glfwInit()){ std::cerr<<"GLFW init failed\n"; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* win = glfwCreateWindow(1920,1080,"Madus Sandbox",nullptr,nullptr);
    if(!win){ glfwTerminate(); return -1; }
    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);

    // capture mouse by default
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
        Vec3 baseRight = Normalize(Cross(baseFwd, Vec3{0,1,0})); // camera right for offsets
        cam.Pos   = Add(hero.Position, Add(Mul(baseFwd, -camBack), Vec3{0, camUp, 0}));
        cam.Yaw   = baseYaw;   
        cam.Pitch = basePitch;

        if (glfwGetKey(win, GLFW_KEY_RIGHT) == GLFW_PRESS) targetOffX += panSpeed * dt;
        if (glfwGetKey(win, GLFW_KEY_LEFT)  == GLFW_PRESS) targetOffX -= panSpeed * dt;
        if (glfwGetKey(win, GLFW_KEY_UP)    == GLFW_PRESS) targetOffY += panSpeed * dt;
        if (glfwGetKey(win, GLFW_KEY_DOWN)  == GLFW_PRESS) targetOffY -= panSpeed * dt;

        // Clamp
        targetOffX = std::clamp(targetOffX, -maxHoriz, +maxHoriz);
        targetOffY = std::clamp(targetOffY, -maxVert,  +maxVert);

        // Spring back to center when keys not pressed
        auto Spring01 = [](float v, float dt, float halfLife){
            if (halfLife <= 0.f) return v;
            float k = 1.f - std::exp(-std::log(2.f) * dt / halfLife);
            return v * (1.f - k);
        };
        bool anyH = (glfwGetKey(win, GLFW_KEY_LEFT)==GLFW_PRESS) || (glfwGetKey(win, GLFW_KEY_RIGHT)==GLFW_PRESS);
        bool anyV = (glfwGetKey(win, GLFW_KEY_UP)==GLFW_PRESS)   || (glfwGetKey(win, GLFW_KEY_DOWN)==GLFW_PRESS);
        if (!anyH) targetOffX = Spring01(targetOffX, dt, springHalfLife);
        if (!anyV) targetOffY = Spring01(targetOffY, dt, springHalfLife);

        //  Build target with distance offsets 
        // World up for vertical pan; camera right for horizontal pan
        Vec3 target = Add(hero.Position, Vec3{0, eyeLift, 0});
        target = Add(target, Mul(baseRight, targetOffX));
        target = Add(target, Mul(Vec3{0,1,0}, targetOffY));

        //  Frame params 
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

        //  SHADOW PASS 
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
        }
        Renderer_Shadow_End();

        // Restore viewport after shadow pass
        glViewport(0,0,w,h);

        // -------------- MAIN PASS --------------
        Renderer_Begin(fp);

        // render sky first
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
        Mat4 Mground = TRS({0,-1,0}, AngleAxis(0,{0,1,0}), {1,1,1});
        Renderer_DrawMesh(plane, sh, Mground, ground);

        // hero proxy
        Mat4 Mhero = TRS(hero.Position, AngleAxis(0,{0,1,0}), {1.0f,1.5f,1.0f});
        Renderer_DrawMesh(box, sh, Mhero, white);

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
