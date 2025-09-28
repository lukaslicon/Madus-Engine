// Copyright Lukas Licon 2025, All Rights Reservered.

#pragma once

struct InputState {
    float MoveX = 0.f; // A/D
    float MoveZ = 0.f; // W/S
    float MouseDX = 0.f;
    float MouseDY = 0.f;
    bool  Jump = false;
    bool  Dash = false;
    void ClearFrameDeltas(){ MouseDX=MouseDY=0; Jump=false; Dash=false; }
};

// Bind to an existing GLFWwindow* (pass from Sandbox game)
void Input_BindWindow(void* glfwWindow);
// Poll keys/mouse and fill InputState (call once/frame)
void Input_Poll(InputState& out);

// Used for capturing and reseting mouse
void Input_ResetMouse();

//active state of game from input
void Input_SetActive(bool active);
bool Input_IsActive();
