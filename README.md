# Madus-Engine

Madus-Engine is a **tiny C++ sandbox engine** built from scratch for learning and prototyping **2D/anime-style games**.  
It uses **GLFW**, **glad**, and **OpenGL 3.3 Core**.  

Current demo: opens a window and draws a soft purple triangle.

![Example Output](image.png)

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

1. **Bootstrap once** (sets up vcpkg and configures the build):
   ```powershell
   & .\tools\setup.ps1
