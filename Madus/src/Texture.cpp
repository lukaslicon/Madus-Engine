// Copyright Lukas Licon 2025, All Rights Reserved.

#include "Madus/Texture.h"
#include <glad/glad.h>
#include <vector>          // <-- needed for CreateCheckerTexture
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <iostream>

unsigned CreateTexture2DWhite(){
    unsigned t=0; glGenTextures(1,&t); glBindTexture(GL_TEXTURE_2D,t);
    unsigned char w[4]={255,255,255,255};
    glTexImage2D(GL_TEXTURE_2D,0,GL_SRGB8_ALPHA8,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,w);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    return t;
}
unsigned CreateTexture2DFromFile(const char* path, bool srgb){
    int w,h,n; stbi_set_flip_vertically_on_load(1);
    unsigned char* d = stbi_load(path,&w,&h,&n,4);
    if(!d){ std::cerr<<"Failed to load texture: "<<path<<"\n"; return CreateTexture2DWhite(); }
    unsigned t=0; glGenTextures(1,&t); glBindTexture(GL_TEXTURE_2D,t);
    glTexImage2D(GL_TEXTURE_2D,0,(srgb?GL_SRGB8_ALPHA8:GL_RGBA8),w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,d);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    stbi_image_free(d);
    return t;
}
void DestroyTexture(unsigned& t){ if(t){ glDeleteTextures(1,&t); t=0; } }


unsigned CreateCheckerTexture(int size, int checks, bool srgb){
    const int comp = 4;
    std::vector<unsigned char> pixels(size*size*comp);
    for(int y=0;y<size;++y){
        for(int x=0;x<size;++x){
            int cx = (x * checks / size);
            int cy = (y * checks / size);
            bool odd = ((cx + cy) & 1) != 0;
            unsigned char c = odd ? 200 : 120; // two grays
            pixels[(y*size + x)*4 + 0] = c;
            pixels[(y*size + x)*4 + 1] = c;
            pixels[(y*size + x)*4 + 2] = c;
            pixels[(y*size + x)*4 + 3] = 255;
        }
    }
    unsigned t=0; glGenTextures(1,&t); glBindTexture(GL_TEXTURE_2D,t);
    glTexImage2D(GL_TEXTURE_2D,0,(srgb?GL_SRGB8_ALPHA8:GL_RGBA8),size,size,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    return t;
}