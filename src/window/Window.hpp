#pragma once

#include <GLFW/glfw3.h>
#include <functional>
#include <string>

class Window {
public:
  /** iconPath: caminho para PNG do ícone (dock/barra); vazio = tenta assets/ramterm-logo.png. */
  Window(int width, int height, const std::string& title, const std::string& iconPath = "");
  ~Window();
  Window(const Window &) = delete;
  Window &operator=(const Window &) = delete;
  bool shouldClose() const;
  void pollEvents() const;
  void waitEventsTimeout(double timeoutSeconds) const;
  void swapBuffers() const;
  int getWidth() const;
  int getHeight() const;
  /** Tamanho da janela em coordenadas de tela (para mapear cursor → célula). */
  int getWindowWidth() const;
  int getWindowHeight() const;
  bool isIconified() const;
  std::string consumeInputBuffer();
  double consumeScrollDelta();

  void setClipboardString(const std::string& text) const;
  std::string getClipboardString() const;

  using MouseButtonCallback = std::function<void(double x, double y, int button, int action)>;
  using CursorPosCallback = std::function<void(double x, double y)>;
  using VoidCallback = std::function<void()>;
  /** delta: +1 aumentar fonte, -1 diminuir (Ctrl+Plus / Ctrl+Minus). */
  using FontZoomCallback = std::function<void(int delta)>;

  void setMouseButtonCallback(MouseButtonCallback cb) { mouseButtonCallback_ = std::move(cb); }
  void setCursorPosCallback(CursorPosCallback cb) { cursorPosCallback_ = std::move(cb); }
  void setCopyCallback(VoidCallback cb) { copyCallback_ = std::move(cb); }
  void setPasteCallback(VoidCallback cb) { pasteCallback_ = std::move(cb); }
  void setFontZoomCallback(FontZoomCallback cb) { fontZoomCallback_ = std::move(cb); }
  /** Chamado quando a janela ganha foco (ex.: usuário clicou); útil para forçar redraw. */
  void setWindowFocusCallback(VoidCallback cb) { windowFocusCallback_ = std::move(cb); }

private:
  static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
  static void charCallback(GLFWwindow *window, unsigned int codepoint);
  static void scrollCallback(GLFWwindow *window, double xoffset, double yoffset);
  static void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
  static void cursorPosCallback(GLFWwindow *window, double x, double y);
  static void framebufferSizeCallback(GLFWwindow *window, int width, int height);
  static void windowFocusCallback(GLFWwindow *window, int focused);

  GLFWwindow *handle_ = nullptr;
  mutable int width_ = 0;
  mutable int height_ = 0;
  std::string inputBuffer_;
  double scrollDelta_ = 0.0;
  MouseButtonCallback mouseButtonCallback_;
  CursorPosCallback cursorPosCallback_;
  VoidCallback copyCallback_;
  VoidCallback pasteCallback_;
  FontZoomCallback fontZoomCallback_;
  VoidCallback windowFocusCallback_;
};