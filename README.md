# Madus-Engine

Madus-Engine is a EARLY WORK IN PROGRESS. **tiny C++ sandbox engine** built from scratch for learning and prototyping games. 

It uses **GLFW**, **glad**, and **OpenGL 3.3 Core**.  

Current demo: opens a 1920x1080 window with a 3D controlable Cube. WASD movement with a dash if you click Shift. Isometric view and colliders are set up.(future setup)

Playing around with light, shaders, rendering, meshes at the moment.

![Example Progress Output](isoimage1.png.png)
<video controls width="320">
  <source src="recording2.mp4" type="video/mp4">
  Your browser does not support the video tag.
</video>

---

## Features (so far)

- 🔹 Minimal C++20 engine core (`Madus/`)  
- 🔹 Cross-platform windowing + input via **GLFW**  
- 🔹 OpenGL function loading via **glad**  
- 🔹 Sandbox app (`Sandbox/`) with a custom `IApp` implementation  
- 🔹 Clean out-of-source builds (`out/build/...`)  
- 🔹 Preset-based CMake workflow (no manual paths)  
- 🔹 PowerShell bootstrap script (`tools/setup.ps1`) for easy setup  

---

## Prerequisites

- **Visual Studio 2022** with *Desktop Development with C++* workload  
- **CMake 3.20+**  
- **vcpkg** (recommended in `%USERPROFILE%\vcpkg`)  
- Run from **Developer PowerShell for VS 2022** (ensures `cl.exe` is on PATH)

If you don’t have vcpkg yet:
```powershell
git clone https://github.com/microsoft/vcpkg $env:USERPROFILE\vcpkg
& $env:USERPROFILE\vcpkg\bootstrap-vcpkg.bat

---

## Quick Start

From the repo root:

```powershell
# Bootstrap once (sets up vcpkg and configures the build) do not need to do this everytime
& .\tools\setup.ps1

# Build the project
cmake --build --preset debug

# Run the executable (path after build)
out\build\vs2022-x64\Sandbox\Debug\MadusSandbox.exe

# Or build and run in one step
cmake --build --preset debug --target run
