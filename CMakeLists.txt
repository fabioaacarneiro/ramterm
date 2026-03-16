cmake_minimum_required(VERSION 3.20)

project(RamTerm VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
find_package(OpenGL REQUIRED)

# yaml-cpp: find_package falha no macOS/Homebrew; usar PkgConfig como fallback
find_package(yaml-cpp QUIET)
if(NOT yaml-cpp_FOUND)
  pkg_check_modules(YAML_CPP REQUIRED yaml-cpp)
  add_library(yaml-cpp::yaml-cpp INTERFACE IMPORTED)
  set_target_properties(yaml-cpp::yaml-cpp PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${YAML_CPP_INCLUDE_DIRS}"
    INTERFACE_LINK_LIBRARIES "${YAML_CPP_LIBRARIES}"
    INTERFACE_LINK_DIRECTORIES "${YAML_CPP_LIBRARY_DIRS}"
  )
endif()

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
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
    ${GLFW_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
    ${VTERM_INCLUDE_DIRS}
)

target_link_libraries(ramterm PRIVATE
    OpenGL::GL
    yaml-cpp::yaml-cpp
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