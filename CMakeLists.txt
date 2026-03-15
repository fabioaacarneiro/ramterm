cmake_minimum_required(VERSION 3.20)

project(RamTerm VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
find_package(OpenGL REQUIRED)
find_package(yaml-cpp REQUIRED)

pkg_check_modules(GLFW REQUIRED glfw3)
pkg_check_modules(FREETYPE REQUIRED freetype2)
pkg_check_modules(VTERM REQUIRED vterm)

add_executable(ramterm
    src/main.cpp
    src/app/Application.cpp
    src/window/Window.cpp
    src/config/Config.cpp
    src/renderer/Renderer.cpp
    src/font/FontManager.cpp
    src/pty/PtyProcess.cpp
    src/terminal/VTermScreenBuffer.cpp
    src/terminal/VTermEngine.cpp
)

target_include_directories(ramterm PRIVATE
    src
    ${GLFW_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
    ${VTERM_INCLUDE_DIRS}
)

target_link_libraries(ramterm PRIVATE
    OpenGL::GL
    yaml-cpp
    ${GLFW_LIBRARIES}
    ${FREETYPE_LIBRARIES}
    ${VTERM_LIBRARIES}
)

target_compile_options(ramterm PRIVATE
    ${GLFW_CFLAGS_OTHER}
    ${FREETYPE_CFLAGS_OTHER}
    ${VTERM_CFLAGS_OTHER}
    -Wall
    -Wextra
    -Wpedantic
)