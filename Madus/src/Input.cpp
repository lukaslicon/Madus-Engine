// Copyright Lukas Licon 2025, All Rights Reserved.

#include "Madus/Input.h"
#include <GLFW/glfw3.h>

static GLFWwindow* GWin = nullptr;
static double LastX = 0.0, LastY = 0.0;
static bool   FirstMouse = true;
static bool GActive = true;

void Input_BindWindow(void* glfwWindow){
    GWin = (GLFWwindow*)glfwWindow;
    FirstMouse = true;
}

void Input_Poll(InputState& o){
    if (!GWin) return;

    if (!GActive || glfwGetWindowAttrib(GWin, GLFW_FOCUSED) == GLFW_FALSE){
        o = {};          
        FirstMouse = true; 
        return;
    }

    // movement (WASD)
    o.MoveX = (glfwGetKey(GWin, GLFW_KEY_D)==GLFW_PRESS ? 1.f : 0.f)
            - (glfwGetKey(GWin, GLFW_KEY_A)==GLFW_PRESS ? 1.f : 0.f);
    o.MoveZ = (glfwGetKey(GWin, GLFW_KEY_W)==GLFW_PRESS ? 1.f : 0.f)
            - (glfwGetKey(GWin, GLFW_KEY_S)==GLFW_PRESS ? 1.f : 0.f);

    // actions
    o.Jump = (glfwGetKey(GWin, GLFW_KEY_SPACE)==GLFW_PRESS);
    o.Dash = (glfwGetKey(GWin, GLFW_KEY_LEFT_SHIFT)==GLFW_PRESS) ||
             (glfwGetKey(GWin, GLFW_KEY_RIGHT_SHIFT)==GLFW_PRESS);

    // mouse delta
    double x,y; glfwGetCursorPos(GWin, &x,&y);
    if (FirstMouse){ LastX=x; LastY=y; FirstMouse=false; }
    o.MouseDX = float(x - LastX);
    o.MouseDY = float(y - LastY);
    LastX=x; LastY=y;

    
    
}

void Input_ResetMouse(){ FirstMouse = true; }
void Input_SetActive(bool active){ GActive = active; }
bool Input_IsActive(){ return GActive; }
