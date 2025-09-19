add_library(Madus STATIC
    src/Engine.cpp
)
target_include_directories(Madus PUBLIC include)

find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glad   CONFIG REQUIRED)   # NEW

target_link_libraries(Madus
    PUBLIC
        glad::glad                   # NEW (PUBLIC so Sandbox can include <glad/gl.h>)
        glfw
        OpenGL::GL
)

if (MSVC)
  target_compile_options(Madus PRIVATE /W4 /permissive-)
endif()
