#include "Madus/Engine.h"
#include "Madus/App.h"

// We can include glad here because Madus PUBLIC-links glad::glad
#include <glad/gl.h>

#include <string>
#include <vector>

using namespace madus;

static unsigned Compile(GLenum type, const char* src) {
    unsigned s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    int ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        int len = 0; glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, '\0');
        glGetShaderInfoLog(s, len, nullptr, log.data());
        OutputDebugStringA(("Shader compile error:\n" + log + "\n").c_str());
    }
    return s;
}

static unsigned Link(unsigned vs, unsigned fs) {
    unsigned p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    int ok = 0; glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        int len = 0; glGetProgramiv(p, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, '\0');
        glGetProgramInfoLog(p, len, nullptr, log.data());
        OutputDebugStringA(("Program link error:\n" + log + "\n").c_str());
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return p;
}

class TriangleApp : public IApp {
public:
    void OnStartup() override {
        // Triangle vertices (x, y). NDC coordinates.
        float verts[] = {
            -0.5f, -0.5f,
             0.5f, -0.5f,
             0.0f,  0.5f
        };

        glGenVertexArrays(1, &mVAO);
        glGenBuffers(1, &mVBO);
        glBindVertexArray(mVAO);
        glBindBuffer(GL_ARRAY_BUFFER, mVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

        const char* vsSrc = R"(#version 330 core
            layout(location=0) in vec2 aPos;
            void main() {
                gl_Position = vec4(aPos, 0.0, 1.0);
            })";

        const char* fsSrc = R"(#version 330 core
            out vec4 FragColor;
            void main() {
                // soft anime-ish purple
                FragColor = vec4(0.82, 0.72, 0.96, 1.0);
            })";

        unsigned vs = Compile(GL_VERTEX_SHADER,   vsSrc);
        unsigned fs = Compile(GL_FRAGMENT_SHADER, fsSrc);
        mProg = Link(vs, fs);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void OnRender() override {
        glUseProgram(mProg);
        glBindVertexArray(mVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
    }

    void OnShutdown() override {
        if (mProg) glDeleteProgram(mProg);
        if (mVBO)  glDeleteBuffers(1, &mVBO);
        if (mVAO)  glDeleteVertexArrays(1, &mVAO);
    }

private:
    unsigned mVAO = 0, mVBO = 0, mProg = 0;
};

int main() {
    EngineConfig cfg;
    cfg.title  = "Madus Sandbox — Triangle";
    cfg.width  = 1280;
    cfg.height = 720;
    cfg.vsync  = true;

    TriangleApp app;
    Engine engine(cfg, &app);
    return engine.Run();
}
