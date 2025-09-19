//Copyright Lukas Licon 2025

#pragma once
#include <cstdint>

struct GLFWwindow;
namespace madus { class IApp; }

namespace madus {

struct EngineConfig {
    int width  = 1280;
    int height = 720;
    const char* title = "Madus Sandbox";
    bool vsync = true;
};

class Engine {
public:
    explicit Engine(const EngineConfig& cfg = {}, IApp* app = nullptr);
    ~Engine();

    int Run();

private:
    bool InitPlatform(const EngineConfig& cfg);
    void ShutdownPlatform();
    void PumpEvents(bool& shouldClose);
    void Update(double dt);
    void Render();

    double m_LastTime = 0.0;

    struct Impl { GLFWwindow* Window = nullptr; };
    Impl* m = nullptr;

    IApp* m_App = nullptr;
};

} // namespace madus
