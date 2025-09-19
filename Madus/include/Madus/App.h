//Copyright Lukas Licon 2025

#pragma once
namespace madus {
class IApp {
public:
    virtual ~IApp() = default;
    virtual void OnStartup() {}
    virtual void OnShutdown() {}
    virtual void OnUpdate(double) {}
    virtual void OnRender() {}
};
} // namespace madus
