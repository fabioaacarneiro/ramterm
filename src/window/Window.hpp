#pragma once

#include <GLFW/glfw3.h>
#include <string>

class Window {
public:
  Window(int width, int height, const std::string &title);
  ~Window();
  Window(const Window &) = delete;
  Window &operator=(const Window &) = delete;
  bool shouldClose() const;
  void pollEvents() const;
  /** Bloqueia até evento ou timeout (segundos); evita CPU 100% quando ocioso. */
  void waitEventsTimeout(double timeoutSeconds) const;
  void swapBuffers() const;
  int getWidth() const;
  int getHeight() const;
  bool isIconified() const;
  std::string consumeInputBuffer();
  double consumeScrollDelta();

private:
  GLFWwindow *handle_;
  mutable int width_;
  mutable int height_;
  std::string inputBuffer_;
  double scrollDelta_;
  static void keyCallback(GLFWwindow *window, int key, int scancode, int action,
                          int mods);
  static void charCallback(GLFWwindow *window, unsigned int codepoint);
  static void scrollCallback(GLFWwindow *window, double xoffset,
                             double yoffset);
  static void framebufferSizeCallback(GLFWwindow *window, int width,
                                      int height);
};