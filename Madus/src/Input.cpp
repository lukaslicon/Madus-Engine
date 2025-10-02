// Copyright Lukas Licon 2025, All Rights Reserved.

#include "Madus/Input.h"
#include <GLFW/glfw3.h>
#include <cmath>
#include <algorithm> 

static GLFWwindow* GWin = nullptr;
static double LastX = 0.0, LastY = 0.0;
static bool   FirstMouse = true;
static bool   GActive = true;

void Input_BindWindow(void* glfwWindow){
    GWin = (GLFWwindow*)glfwWindow;
    FirstMouse = true;
}

void Input_Poll(InputState& o){
    if (!GActive || !GWin) return;

    // --- Movement axes (WASD) ---
    const int w = glfwGetKey(GWin, GLFW_KEY_W);
    const int s = glfwGetKey(GWin, GLFW_KEY_S);
    const int a = glfwGetKey(GWin, GLFW_KEY_A);
    const int d = glfwGetKey(GWin, GLFW_KEY_D);

    //Z movement
    o.MoveZ = 0.f;
    if (w == GLFW_PRESS) o.MoveZ += 1.f;
    if (s == GLFW_PRESS) o.MoveZ -= 1.f;

    //X movement
    o.MoveX = 0.f;
    if (d == GLFW_PRESS) o.MoveX += 1.f;
    if (a == GLFW_PRESS) o.MoveX -= 1.f;

    //input calc
    const float len = std::sqrt(o.MoveX*o.MoveX + o.MoveZ*o.MoveZ);
    if (len > 1e-6f){
        const float inv = 1.0f / std::max(1.0f, len);
        o.MoveX *= inv;
        o.MoveZ *= inv;
    }

    //Jump
    o.Jump = (glfwGetKey(GWin, GLFW_KEY_SPACE) == GLFW_PRESS);

    //Dash
    o.Dash = (glfwGetKey(GWin, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ||
             (glfwGetKey(GWin, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);

    //abilities options for later
    o.AbilityQ = (glfwGetKey(GWin, GLFW_KEY_Q) == GLFW_PRESS);
    o.AbilityE = (glfwGetKey(GWin, GLFW_KEY_E) == GLFW_PRESS);
    o.AbilityR = (glfwGetKey(GWin, GLFW_KEY_R) == GLFW_PRESS);
    o.Ability1 = (glfwGetKey(GWin, GLFW_KEY_1) == GLFW_PRESS);
    o.Ability2 = (glfwGetKey(GWin, GLFW_KEY_2) == GLFW_PRESS);
    o.Ability3 = (glfwGetKey(GWin, GLFW_KEY_3) == GLFW_PRESS);
    o.Ability4 = (glfwGetKey(GWin, GLFW_KEY_4) == GLFW_PRESS);

    // right input (can decide later for like npc interaction)
    o.InputRMB = (glfwGetMouseButton(GWin, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);


    //Cursor positioning (for spells)
    double x, y;
    glfwGetCursorPos(GWin, &x, &y);
    if (FirstMouse){ LastX = x; LastY = y; FirstMouse = false; }
    o.MouseDX = float(x - LastX);
    o.MouseDY = float(y - LastY);
    LastX = x; LastY = y;
}

void Input_ResetMouse(){ FirstMouse = true; }
void Input_SetActive(bool active){ GActive = active; }
bool Input_IsActive(){ return GActive; }
