# Madus-Engine

Tiny C++ game engine sandboxed for **2D/anime-style** rendering.  
Current demo: opens a window and draws a triangle (GLFW + glad + OpenGL 3.3).

![Example Output](image.png)

---

## Prerequisites (Windows)

- **Visual Studio 2022** with **Desktop development with C++**
- **CMake 3.20+**
- **vcpkg** (any install location, e.g. `%USERPROFILE%\vcpkg`)
- Use **Developer PowerShell for VS 2022** (so `cl.exe` is on PATH)

> If you don’t have vcpkg:
> ```powershell
> git clone https://github.com/microsoft/vcpkg $env:USERPROFILE\vcpkg
> & $env:USERPROFILE\vcpkg\bootstrap-vcpkg.bat
> ```

---

## Dependencies (via vcpkg)

```powershell
$env:VCPKG_ROOT = "$env:USERPROFILE\vcpkg"
& "$env:VCPKG_ROOT\vcpkg.exe" install glfw3:x64-windows
& "$env:VCPKG_ROOT\vcpkg.exe" install glad:x64-windows
# (Optional, for textures later)
# & "$env:VCPKG_ROOT\vcpkg.exe" install stb:x64-windows
