// Copyright Lukas Licon 2025, All Rights Reservered.

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include "Madus/Math.h"
#include "Madus/Camera.h"
#include "Madus/Input.h"
#include "Madus/Mesh.h"
#include "Madus/Texture.h"
#include "Madus/Renderer.h"
#include "Madus/CharacterController.h"

// Optional GL debug callback
static void GLAPIENTRY glDbg(GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar* msg, const void*) {
    std::cerr << "[GL] " << msg << "\n";
}

// tiny Mat4 multiply helper (column-major)
static Mat4 MulM(const Mat4& A, const Mat4& B){
    Mat4 R{};
    for(int c=0;c<4;++c)
        for(int r=0;r<4;++r)
            R.m[c*4+r] = A.m[0*4+r]*B.m[c*4+0] + A.m[1*4+r]*B.m[c*4+1]
                       + A.m[2*4+r]*B.m[c*4+2] + A.m[3*4+r]*B.m[c*4+3];
    return R;
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

    // Scene
    Camera cam;
    cam.Pos = {0, 2.0f, 8.0f};

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

        // mouse look (adjust sensitivity)
        if (Input_IsActive()) {
            cam.AddYawPitch(-in.MouseDX * 0.0025f, -in.MouseDY * 0.0020f);
        }

        // RMB fly
        const float camMove = 7.0f * dt;
        if (gMouseCaptured && glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_RIGHT)==GLFW_PRESS){
            cam.Pos = Add(cam.Pos, Mul(cam.Forward(),  (in.MoveZ)*camMove));
            cam.Pos = Add(cam.Pos, Mul(cam.Right(),    (in.MoveX)*camMove));
        }

        // Press ESC to release the cursor
        if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS && gMouseCaptured) {
            glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            if (glfwRawMouseMotionSupported()) glfwSetInputMode(win, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
            gMouseCaptured = false;
            Input_SetActive(false);
            Input_ResetMouse();
        }
        // Press Left Mouse to recapture cursor
        if (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && !gMouseCaptured) {
            glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            if (glfwRawMouseMotionSupported()) glfwSetInputMode(win, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
            gMouseCaptured = true;
            Input_SetActive(true);
            Input_ResetMouse();
        }

        // character tick (relative to camera forward/right)
        if (Input_IsActive()) {
            hero.Tick(in, dt, cam.Forward(), cam.Right());
        }

        // simple third-person follow (offset behind and above hero)
        Vec3 follow = Add(hero.Position, Add(Mul(cam.Forward(), -6.f), Vec3{0,3.0f,0}));
        cam.Pos = follow;

        // Frame params
        FrameParams fp{};
        fp.View = cam.View();
        fp.Proj = cam.Proj((float)w/(float)h);
        fp.Sun  = DirectionalLight{};
        fp.Sun.dir[0] = -0.35f; fp.Sun.dir[1] = -0.90f; fp.Sun.dir[2] = -0.20f;
        fp.Sun.intensity = 3.0f;

        // -------- Sun direction (single source for all passes) --------
        Vec3 sunDir = Normalize(Vec3{ fp.Sun.dir[0], fp.Sun.dir[1], fp.Sun.dir[2] });
        // keep everyone in sync (sky, lighting, shadows)
        fp.Sun.dir[0] = sunDir.x;
        fp.Sun.dir[1] = sunDir.y;
        fp.Sun.dir[2] = sunDir.z;

        // -------- SHADOW PASS (tight ortho, centered on hero) --------
        Vec3 center = hero.Position; center.y = 0.0f;
        float lightDist = 30.0f;
        Vec3 lightPos = Add(center, Mul(sunDir, -lightDist));  // center - dir * dist

        Mat4 LView = LookAt(lightPos, center, {0,1,0});
        float R = 18.0f; // tighter box = sharper shadows
        Mat4 LProj = Ortho(-R, R, -R, R, 0.1f, 80.0f);

        ShadowMapInfo sm{ LView, LProj, 2048 };
        Renderer_Shadow_Begin(sm);
        {
            Renderer_Shadow_DrawDepth(plane, TRS({0,0,0}, AngleAxis(0,{0,1,0}), {1,1,1}));
            for(int i=0;i<6;++i){
                float x = -6.f + i*2.4f;
                Mat4 M = TRS({x, 0.5f, -4.f}, AngleAxis(0,{0,1,0}), {1,1,1});
                Renderer_Shadow_DrawDepth(box, M);
            }
            Renderer_Shadow_DrawDepth(box, TRS(hero.Position, AngleAxis(0,{0,1,0}), {1,1,1}));
        }
        Renderer_Shadow_End();

        // Restore viewport after shadow pass
        glViewport(0,0,w,h);

        // -------- MAIN PASS --------
        Renderer_Begin(fp);

        // draw sky FIRST (uses fp.Sun.dir which we set to sunDir)
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

        // test cubes
        for(int i=0;i<6;++i){
            float x = -6.f + i*2.4f;
            Mat4 M = TRS({x, 0.5f, -4.f}, AngleAxis(0,{0,1,0}), {1,1,1});
            Renderer_DrawMesh(box, sh, M, white);
        }

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
