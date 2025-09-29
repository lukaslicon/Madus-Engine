// Copyright Lukas Licon 2025, All Rights Reserved.

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
} 
