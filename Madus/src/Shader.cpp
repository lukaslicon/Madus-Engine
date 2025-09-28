// Copyright Lukas Licon 2025, All Rights Reservered.

#include "Madus/Shader.h"
#include <glad/glad.h>
#include <iostream>

static unsigned CompileStage(unsigned type, const char* src){
    unsigned s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    int ok=0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if(!ok){ char log[2048]; glGetShaderInfoLog(s, sizeof(log), nullptr, log); std::cerr<<"Shader compile error:\n"<<log<<"\n"; }
    return s;
}
ShaderHandle CreateShaderProgram(const char* vsSrc, const char* fsSrc){
    unsigned vs = CompileStage(GL_VERTEX_SHADER, vsSrc);
    unsigned fs = CompileStage(GL_FRAGMENT_SHADER, fsSrc);
    unsigned p = glCreateProgram();
    glAttachShader(p, vs); glAttachShader(p, fs);
    glLinkProgram(p);
    int ok=0; glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if(!ok){ char log[2048]; glGetProgramInfoLog(p, sizeof(log), nullptr, log); std::cerr<<"Program link error:\n"<<log<<"\n"; }
    glDeleteShader(vs); glDeleteShader(fs);
    return p;
}
void DestroyShaderProgram(ShaderHandle h){ if(h) glDeleteProgram(h); }
int  GetUniformLocation(ShaderHandle h, const char* name){ return glGetUniformLocation(h, name); }
