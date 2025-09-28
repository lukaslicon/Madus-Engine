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

    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(win, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    bool gMouseCaptured = true;

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
    Renderer_Shadow_Init(2048);

    int w=1920,h=1080;
    Renderer_Resize(w,h);

    // Scene bits
    Camera cam;
    cam.Pos = {0, 2.0f, 8.0f};

    GpuMesh plane = CreatePlane(40.f);
    GpuMesh box   = CreateBoxUnit();
    unsigned ground = CreateCheckerTexture(1024, 16, true);
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
        // Press ESC to release the cursor
        if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS && gMouseCaptured) {
            glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            gMouseCaptured = false;
            Input_SetActive(false);   // <- disable game input
            Input_ResetMouse();
        }       

        // Press Left Mouse to recapture cursor
        if (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && !gMouseCaptured) {
            glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            gMouseCaptured = true;
            Input_SetActive(true);    // <- enable game input
            Input_ResetMouse();
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
        fp.Sun.dir[0] = -0.35f; fp.Sun.dir[1] = -0.9f; fp.Sun.dir[2] = -0.2f;
        fp.Sun.intensity = 3.0f;

        // --- Build light matrices for the sun ---
        Vec3 lightDir = { fp.Sun.dir[0], fp.Sun.dir[1], fp.Sun.dir[2] };
        Vec3 center   = { 0, 0, 0 };        // focus area (you can track hero)
        Vec3 lightPos = Add(center, Mul(lightDir, -30.0f)); // back along dir

        Mat4 LView = LookAt(lightPos, center, {0,1,0});
        // Fit your scene in this box. Adjust as needed.
        Mat4 LProj = Ortho(-20, 20, -20, 20, 0.1f, 80.0f);

        ShadowMapInfo sm{ LView, LProj, 2048 };
        Renderer_Shadow_Begin(sm);

        // draw the same geometry as depth-only
        Renderer_Shadow_DrawDepth(plane, TRS({0,0,0}, AngleAxis(0,{0,1,0}), {1,1,1}));
        for(int i=0;i<6;++i){
            float x = -6.f + i*2.4f;
            Mat4 M = TRS({x, 0.5f, -4.f}, AngleAxis(0,{0,1,0}), {1,1,1});
            Renderer_Shadow_DrawDepth(box, M);
        }
        Renderer_Shadow_DrawDepth(box, TRS(hero.Position, AngleAxis(0,{0,1,0}), {1,1,1}));

        Renderer_Shadow_End();
        Renderer_Begin(fp);
        glUseProgram(sh);

        // Set common uniforms once for this shader
        glUseProgram(sh);

        int locV = GetUniformLocation(sh,"uView");
        int locP = GetUniformLocation(sh,"uProj");
        int locDir = GetUniformLocation(sh,"uSunDir");
        int locCol = GetUniformLocation(sh,"uSunColor");
        int locInt = GetUniformLocation(sh,"uSunIntensity");
        int locCam = GetUniformLocation(sh,"uCamPos");
        int locSky = GetUniformLocation(sh,"uSkyColor");
        int locGnd = GetUniformLocation(sh,"uGroundColor");
        int locSh  = GetUniformLocation(sh,"uShadowMap");
        int locLVP = GetUniformLocation(sh,"uLightVP");

        // Shadow Light computations
        Mat4 LightVP; // compute here
        auto MulM = [](const Mat4& A, const Mat4& B){
            Mat4 R{}; 
            for(int c=0;c<4;++c)
                for(int r=0;r<4;++r)
                    R.m[c*4+r] = A.m[0*4+r]*B.m[c*4+0] + A.m[1*4+r]*B.m[c*4+1] + A.m[2*4+r]*B.m[c*4+2] + A.m[3*4+r]*B.m[c*4+3];
            return R;
        };
        LightVP = MulM(LProj, LView);

        glUniformMatrix4fv(locLVP, 1, GL_FALSE, LightVP.m);

        // bind shadow map to texture unit 1 (albedo is 0)
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, Renderer_Shadow_GetTexture());
        glUniform1i(locSh, 1);
        glActiveTexture(GL_TEXTURE0); // back to albedo for draws


        // cam
        glUniform3f(locCam, cam.Pos.x, cam.Pos.y, cam.Pos.z);
        // sky bluish 
        glUniform3f(locSky, 0.32f, 0.42f, 0.62f); // slightly dimmer sky
        glUniform3f(locGnd, 0.10f, 0.09f, 0.09f); // darker ground ambient

        glUniformMatrix4fv(locV,1,GL_FALSE, fp.View.m);
        glUniformMatrix4fv(locP,1,GL_FALSE, fp.Proj.m);

        //sun dir
        glUniform3f(locDir, fp.Sun.dir[0], fp.Sun.dir[1], fp.Sun.dir[2]);
        //sun color
        glUniform3f(locCol, fp.Sun.color[0], fp.Sun.color[1], fp.Sun.color[2]);
        //sun intensity (bright)
        glUniform1f(locInt, fp.Sun.intensity);


        // draw ground
        Mat4 Mground = TRS({0,0,0}, AngleAxis(0,{0,1,0}), {1,1,1});
        Renderer_DrawMesh(plane, sh, Mground, ground);

        // few testing boxes
        for(int i=0;i<6;++i){
            float x = -6.f + i*2.4f;
            Mat4 M = TRS({x, 0.5f, -4.f}, AngleAxis(0,{0,1,0}), {1,1,1});
            Renderer_DrawMesh(box, sh, M, white);
        }

        // character hero (1x1x1 box for now)
        Mat4 Mhero = TRS(hero.Position, AngleAxis(0,{0,1,0}), {1.0f,1.0f,1.0f});
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
