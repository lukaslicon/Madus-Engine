// Copyright Lukas Licon 2025, All Rights Reservered.

#pragma once

#include <cmath>
#include <cstdint>
#include <algorithm>

#ifndef MADUS_PI
#define MADUS_PI 3.14159265358979323846
#endif

struct Vec3 { float x=0, y=0, z=0; };
struct Quat { float x=0, y=0, z=0, w=1; }; // (x,y,z) imaginary, w real
struct Mat4 { float m[16]; };              // column-major (OpenGL-style)

inline Mat4 Identity() { Mat4 M{}; for(int i=0;i<16;++i) M.m[i]=(i%5==0)?1.f:0.f; return M; }

Mat4 Perspective(float fovYRadians, float aspect, float zNear, float zFar);
Mat4 Ortho(float l,float r,float b,float t,float n,float f);
Mat4 LookAt(const Vec3& eye, const Vec3& at, const Vec3& up);
Mat4 TRS(const Vec3& t, const Quat& r, const Vec3& s);

inline Vec3  Add(Vec3 a, Vec3 b){ return {a.x+b.x,a.y+b.y,a.z+b.z}; }
inline Vec3  Sub(Vec3 a, Vec3 b){ return {a.x-b.x,a.y-b.y,a.z-b.z}; }
inline Vec3  Mul(Vec3 a, float s){ return {a.x*s,a.y*s,a.z*s}; }
inline float Dot(const Vec3& a, const Vec3& b){ return a.x*b.x+a.y*b.y+a.z*b.z; }
inline Vec3  Cross(const Vec3& a, const Vec3& b){ return {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x}; }
inline float Length(Vec3 v){ return std::sqrt(std::max(0.f, Dot(v,v))); }
inline Vec3  Normalize(Vec3 v){ float L = Length(v); return (L>1e-6f)?Mul(v,1.f/L):Vec3{0,0,0}; }

Quat AngleAxis(float radians, const Vec3& axis);
Mat4 QuatToMat4(const Quat& q);
