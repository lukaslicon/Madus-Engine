// Copyright Lukas Licon 2025, All Rights Reserved.

 #pragma once

 struct InputState {
    float MoveX = 0.f; // A/D
    float MoveZ = 0.f; // W/S
    float MouseDX = 0.f;
    float MouseDY = 0.f;
    bool  AbilityQ = false;  
    bool  AbilityE = false; 
    bool  AbilityR = false;  
    bool  Ability1 = false;
    bool  Ability2 = false;
    bool  Ability3 = false;
    bool  Ability4 = false;
    bool  AttackLMB = false; 
    bool  InputRMB = false;
    bool  Jump = false;
    bool  Dash = false;
    void ClearFrameDeltas(){
        MouseDX = MouseDY = 0;
        Jump = Dash = false;
        AbilityQ = AbilityE = AbilityR = Ability1 = Ability2 = Ability3 = Ability4 = false;
        AttackLMB = false;
    }
 };

 void Input_BindWindow(void* glfwWindow);
 void Input_Poll(InputState& out);
 void Input_ResetMouse();
 void Input_SetActive(bool active);
 bool Input_IsActive();
