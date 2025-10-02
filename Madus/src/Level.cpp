// Copyright Lukas Licon 2025, All Rights Reserved.

#include "Level.h"
#include <fstream>
#include <sstream>
#include <string>
#include <cstdio>

static bool IsCommentOrBlank(const std::string& s){
    // trim left
    size_t i = s.find_first_not_of(" \t\r\n");
    if (i == std::string::npos) return true;
    if (s[i] == '#') return true;
    if (i + 1 < s.size() && s[i] == '/' && s[i+1] == '/') return true;
    return false;
}

bool Level::LoadTxt(const char* path){
    std::ifstream f(path);
    if (!f.is_open()){
        std::printf("[Level] Failed to open '%s'\n", path);
        return false;
    }

    Colliders.clear();
    std::string line;
    int lineno = 0;
    int added = 0;

    while (std::getline(f, line)){
        ++lineno;
        if (IsCommentOrBlank(line)) continue;

        std::istringstream iss(line);
        float minx, minz, maxx, maxz;
        if (!(iss >> minx >> minz >> maxx >> maxz)){
            std::printf("[Level] Parse error at %s:%d -> '%s'\n", path, lineno, line.c_str());
            continue;
        }
        if (maxx < minx) std::swap(maxx, minx);
        if (maxz < minz) std::swap(maxz, minz);

        Colliders.push_back(AABB2{minx, minz, maxx, maxz});
        ++added;
    }

    std::printf("[Level] Loaded %d colliders from '%s'\n", added, path);
    return (added > 0);
}
