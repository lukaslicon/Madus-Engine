# Madus/CMakeLists.txt
cmake_minimum_required(VERSION 3.21)

# This file is intended to be included from the root CMake as a subdirectory.
# It builds the static 'Madus' library consumed by Sandbox.

project(MadusLib LANGUAGES CXX)

# ---- Options ----
option(MADUS_ENABLE_UNITY "Enable unity/jumbo builds for faster compiles" OFF)

# ---- Library ----
add_library(Madus STATIC
    # Sources
    src/Engine.cpp

    # New engine layers
    src/Math.cpp
    src/Input.cpp
    src/Shader.cpp
    src/Mesh.cpp
    src/Texture.cpp
    src/Renderer.cpp
    src/CharacterController.cpp

    # Public headers (not required to list, but helps IDEs)
    include/Madus/App.h
    include/Madus/Engine.h
    include/Madus/Math.h
    include/Madus/Camera.h
    include/Madus/Input.h
    include/Madus/Shader.h
    include/Madus/Mesh.h
    include/Madus/Texture.h
    include/Madus/Renderer.h
    include/Madus/CharacterController.h
)

add_library(Madus::Madus ALIAS Madus)

target_include_directories(Madus
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
)

# ---- Language / Warnings ----
target_compile_features(Madus PUBLIC cxx_std_20)

if (MSVC)
    target_compile_options(Madus PRIVATE /W4 /permissive- /Zc:preprocessor /MP)
else()
    target_compile_options(Madus PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ---- Dependencies (via vcpkg manifest in repo root) ----
# We use GLAD (OpenGL loader) + GLFW (window/input). stb is header-only (no link step).
find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glad   CONFIG REQUIRED)

target_link_libraries(Madus
    PUBLIC
        glad::glad          # exposes <glad/glad.h> + GL function pointers to dependents
        glfw                # GLFW windowing/input
        OpenGL::GL          # Core OpenGL (for GL enums/types on some platforms)
)

# ---- Unity/Jumbo (optional) ----
if (MADUS_ENABLE_UNITY)
    set_target_properties(Madus PROPERTIES UNITY_BUILD ON)
endif()

# ---- Compile definitions (none required) ----
# Note: We define STB_IMAGE_IMPLEMENTATION inside Texture.cpp, so we do NOT define it here.

# ---- Platform specifics ----
if (WIN32 AND MSVC)
    # Prefer static runtime if you decide to ship a single exe later
    string(REPLACE "/MDd" "/MTd" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/MD"  "/MT"  CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
endif()
