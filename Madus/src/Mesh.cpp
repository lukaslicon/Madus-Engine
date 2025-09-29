// Copyright Lukas Licon 2025, All Rights Reserved.

#include "Madus/Mesh.h"
#include <glad/glad.h>
#include <vector>

struct V { float p[3]; float n[3]; float uv[2]; };

static GpuMesh Upload(const std::vector<V>& vtx, const std::vector<uint32_t>& idx){
    GpuMesh g{};
    glGenVertexArrays(1,&g.vao); glBindVertexArray(g.vao);
    glGenBuffers(1,&g.vbo); glBindBuffer(GL_ARRAY_BUFFER, g.vbo);
    glBufferData(GL_ARRAY_BUFFER, vtx.size()*sizeof(V), vtx.data(), GL_STATIC_DRAW);
    glGenBuffers(1,&g.ibo); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size()*sizeof(uint32_t), idx.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(V),(void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(V),(void*)(sizeof(float)*3));
    glEnableVertexAttribArray(2); glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,sizeof(V),(void*)(sizeof(float)*6));
    glBindVertexArray(0);
    g.indexCount = (uint32_t)idx.size();
    return g;
}

GpuMesh CreateBoxUnit(){
    const float s = 0.5f;
    std::vector<V> v = {
        // +X (right)
        {{ s,-s,-s},{ 1,0,0},{0,0}}, {{ s, s,-s},{ 1,0,0},{0,1}}, {{ s, s, s},{ 1,0,0},{1,1}}, {{ s,-s, s},{ 1,0,0},{1,0}},
        // -X (left)
        {{-s,-s, s},{-1,0,0},{0,0}}, {{-s, s, s},{-1,0,0},{0,1}}, {{-s, s,-s},{-1,0,0},{1,1}}, {{-s,-s,-s},{-1,0,0},{1,0}},
        // +Y (top)
        {{-s, s,-s},{0, 1,0},{0,0}}, {{-s, s, s},{0, 1,0},{0,1}}, {{ s, s, s},{0, 1,0},{1,1}}, {{ s, s,-s},{0, 1,0},{1,0}},
        // -Y (bottom)
        {{-s,-s, s},{0,-1,0},{0,0}}, {{-s,-s,-s},{0,-1,0},{0,1}}, {{ s,-s,-s},{0,-1,0},{1,1}}, {{ s,-s, s},{0,-1,0},{1,0}},
        // +Z (front)
        {{-s,-s, s},{0,0, 1},{0,0}}, {{ s,-s, s},{0,0, 1},{1,0}}, {{ s, s, s},{0,0, 1},{1,1}}, {{-s, s, s},{0,0, 1},{0,1}},
        // -Z (back)
        {{ s,-s,-s},{0,0,-1},{0,0}}, {{-s,-s,-s},{0,0,-1},{1,0}}, {{-s, s,-s},{0,0,-1},{1,1}}, {{ s, s,-s},{0,0,-1},{0,1}},
    };
    std::vector<uint32_t> idx; idx.reserve(36);
    for(uint32_t i=0;i<24;i+=4) idx.insert(idx.end(), {i, i+1, i+2,  i, i+2, i+3}); // CCW
    return Upload(v, idx);
}

GpuMesh CreatePlane(float size){
    float s=size*0.5f;
    std::vector<V> v = {
        {{-s,0,-s},{0,1,0},{0,0}},
        {{ s,0,-s},{0,1,0},{1,0}},
        {{ s,0, s},{0,1,0},{1,1}},
        {{-s,0, s},{0,1,0},{0,1}},
    };
    std::vector<uint32_t> i = {0,2,1, 0,3,2};
    return Upload(v,i);
}


void DestroyMesh(GpuMesh& m){
    if(m.ibo) glDeleteBuffers(1,&m.ibo);
    if(m.vbo) glDeleteBuffers(1,&m.vbo);
    if(m.vao) glDeleteVertexArrays(1,&m.vao);
    m = {};
}
