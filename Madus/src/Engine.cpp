#include "Madus/Engine.h"
#include "Madus/App.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <chrono>
#include <cstdio>
#include <cassert>

namespace madus {

static double NowSeconds() {
    using clock = std::chrono::high_resolution_clock;
    return std::chrono::duration<double>(clock::now().time_since_epoch()).count();
}

Engine::Engine(const EngineConfig& cfg, IApp* app) : m(new Impl), m_App(app) {
    const bool ok = InitPlatform(cfg);
    assert(ok && "Madus: platform init failed");
    m_LastTime = NowSeconds();
    if (m_App) m_App->OnStartup();
}

Engine::~Engine() {
    if (m_App) m_App->OnShutdown();
    ShutdownPlatform();
    delete m; m = nullptr;
}

bool Engine::InitPlatform(const EngineConfig& cfg) {
    if (!glfwInit()) { std::fprintf(stderr, "Madus: glfwInit failed\n"); return false; }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    m->Window = glfwCreateWindow(cfg.width, cfg.height, cfg.title, nullptr, nullptr);
    if (!m->Window) { std::fprintf(stderr, "Madus: glfwCreateWindow failed\n"); glfwTerminate(); return false; }

    glfwMakeContextCurrent(m->Window);
    glfwSwapInterval(cfg.vsync ? 1 : 0);

    // Load OpenGL functions
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::fprintf(stderr, "Madus: gladLoadGLLoader failed\n");
        return false;
    }

    int w, h;
    glfwGetFramebufferSize(m->Window, &w, &h);
    glViewport(0, 0, w, h);
    return true;
}

void Engine::ShutdownPlatform() {
    if (m->Window) { glfwDestroyWindow(m->Window); m->Window = nullptr; }
    glfwTerminate();
}

void Engine::PumpEvents(bool& shouldClose) {
    glfwPollEvents();
    shouldClose = glfwWindowShouldClose(m->Window);
}

void Engine::Update(double dt) {
    if (m_App) m_App->OnUpdate(dt);
}

void Engine::Render() {
    glClearColor(0.12f, 0.12f, 0.14f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (m_App) m_App->OnRender();

    glfwSwapBuffers(m->Window);
}

int Engine::Run() {
    bool shouldClose = false;
    while (!shouldClose) {
        const double t  = NowSeconds();
        const double dt = t - m_LastTime;
        m_LastTime = t;

        PumpEvents(shouldClose);
        Update(dt);
        Render();
    }
    return 0;
}

} // namespace madus
