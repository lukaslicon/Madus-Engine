// Copyright Lukas Licon 2025, All Rights Reservered.

#include "Madus/Math.h"

Mat4 Perspective(float fovY, float a, float n, float f){
    float s = 1.f / std::tan(fovY*0.5f);
    Mat4 M{};
    M.m[0]=s/a; M.m[5]=s; M.m[10]=-(f+n)/(f-n); M.m[11]=-1; M.m[14]=-(2.f*f*n)/(f-n);
    return M;
}
Mat4 Ortho(float l,float r,float b,float t,float n,float f){
    Mat4 M = Identity();
    M.m[0]=2.f/(r-l); M.m[5]=2.f/(t-b); M.m[10]=-2.f/(f-n);
    M.m[12]=-(r+l)/(r-l); M.m[13]=-(t+b)/(t-b); M.m[14]=-(f+n)/(f-n);
    return M;
}
Mat4 LookAt(const Vec3& eye, const Vec3& at, const Vec3& up){
    Vec3 f = Normalize(Sub(at,eye));
    Vec3 s = Normalize(Cross(f, up));
    Vec3 u = Cross(s, f);
    Mat4 M = Identity();
    M.m[0]=s.x; M.m[4]=s.y; M.m[8 ]=s.z;
    M.m[1]=u.x; M.m[5]=u.y; M.m[9 ]=u.z;
    M.m[2]=-f.x;M.m[6]=-f.y;M.m[10]=-f.z;
    M.m[12]=-Dot(s,eye); M.m[13]=-Dot(u,eye); M.m[14]= Dot(f,eye);
    return M;
}
Quat AngleAxis(float r, const Vec3& axis){
    Vec3 a = Normalize(axis);
    float s = std::sin(r*0.5f), c = std::cos(r*0.5f);
    return {a.x*s, a.y*s, a.z*s, c};
}
Mat4 QuatToMat4(const Quat& q){
    float x=q.x,y=q.y,z=q.z,w=q.w;
    float xx=x*x, yy=y*y, zz=z*z;
    float xy=x*y, xz=x*z, yz=y*z;
    float wx=w*x, wy=w*y, wz=w*z;
    Mat4 M = Identity();
    M.m[0]=1-2*(yy+zz); M.m[4]=2*(xy- wz); M.m[8 ]=2*(xz+ wy);
    M.m[1]=2*(xy+ wz); M.m[5]=1-2*(xx+zz); M.m[9 ]=2*(yz- wx);
    M.m[2]=2*(xz- wy); M.m[6]=2*(yz+ wx); M.m[10]=1-2*(xx+yy);
    return M;
}
Mat4 TRS(const Vec3& t, const Quat& r, const Vec3& s){
    Mat4 R = QuatToMat4(r);
    R.m[0]*=s.x; R.m[1]*=s.x; R.m[2]*=s.x;
    R.m[4]*=s.y; R.m[5]*=s.y; R.m[6]*=s.y;
    R.m[8]*=s.z; R.m[9]*=s.z; R.m[10]*=s.z;
    R.m[12]=t.x; R.m[13]=t.y; R.m[14]=t.z;
    return R;
}
