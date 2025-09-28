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

static void GLAPIENTRY glDbg(GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar* msg, const void*) {
    std::cerr << "[GL] " << msg << "\n";
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

    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){ std::cerr<<"glad failed\n"; return -1; }
#ifndef NDEBUG
    // Only enable debug output if the function was loaded by glad
    if (glDebugMessageCallback) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(glDbg, nullptr);
    }
#endif

    Input_BindWindow(win);
    Renderer_Init(win);

    int w=1920,h=1080;
    Renderer_Resize(w,h);

    // Scene bits
    Camera cam;
    cam.Pos = {0, 2.0f, 8.0f};

    GpuMesh plane = CreatePlane(40.f);
    GpuMesh box   = CreateBoxUnit();
    unsigned white = CreateTexture2DWhite();

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
        cam.AddYawPitch(-in.MouseDX * 0.0025f, -in.MouseDY * 0.0020f);

        // WASD
        const float camMove = 7.0f * dt;
        if (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_RIGHT)==GLFW_PRESS){
            cam.Pos = Add(cam.Pos, Mul(cam.Forward(),  (in.MoveZ)*camMove));
            cam.Pos = Add(cam.Pos, Mul(cam.Right(),    (in.MoveX)*camMove));
        }

        // character tick (relative to camera forward/right)
        hero.Tick(in, dt, cam.Forward(), cam.Right());

        // simple third-person follow (offset behind and above hero)
        Vec3 follow = Add(hero.Position, Add(Mul(cam.Forward(), -6.f), Vec3{0,3.0f,0}));
        cam.Pos = follow;

        FrameParams fp{};
        fp.View = cam.View();
        fp.Proj = cam.Proj((float)w/(float)h);
        fp.Sun  = DirectionalLight{};
        Renderer_Begin(fp);

        // Set common uniforms once for this shader
        glUseProgram(sh);
        int locV = GetUniformLocation(sh,"uView");
        int locP = GetUniformLocation(sh,"uProj");
        int locDir = GetUniformLocation(sh,"uSunDir");
        int locCol = GetUniformLocation(sh,"uSunColor");
        int locInt = GetUniformLocation(sh,"uSunIntensity");
        glUniformMatrix4fv(locV,1,GL_FALSE, fp.View.m);
        glUniformMatrix4fv(locP,1,GL_FALSE, fp.Proj.m);
        glUniform3f(locDir, fp.Sun.dir[0], fp.Sun.dir[1], fp.Sun.dir[2]);
        glUniform3f(locCol, fp.Sun.color[0], fp.Sun.color[1], fp.Sun.color[2]);
        glUniform1f(locInt, fp.Sun.intensity);

        // draw ground
        Mat4 Mground = TRS({0,0,0}, AngleAxis(0,{0,1,0}), {1,1,1});
        Renderer_DrawMesh(plane, sh, Mground, white);

        // draw a few platforms
        for(int i=0;i<6;++i){
            float x = -6.f + i*2.4f;
            Mat4 M = TRS({x, 0.5f, -4.f}, AngleAxis(0,{0,1,0}), {1,1,1});
            Renderer_DrawMesh(box, sh, M, white);
        }

        // draw hero proxy (scaled box)
        Mat4 Mhero = TRS(hero.Position, AngleAxis(0,{0,1,0}), {0.8f,1.6f,0.8f});
        Renderer_DrawMesh(box, sh, Mhero, white);

        Renderer_End();
        glfwSwapBuffers(win);

        in.ClearFrameDeltas();
    }

    DestroyTexture(white);
    DestroyMesh(box);
    DestroyMesh(plane);
    Renderer_Shutdown();
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
