#include "Window.hpp"
#include <stdexcept>

Window::Window(int width, int height, const std::string &title)
    : handle_(nullptr), width_(width), height_(height), inputBuffer_(),
      scrollDelta_(0.0) {
  if (!glfwInit()) {
    throw std::runtime_error("Falha ao iniciar GLFW.");
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  glfwWindowHintString(GLFW_X11_CLASS_NAME, "RamTerm");
  glfwWindowHintString(GLFW_X11_INSTANCE_NAME, "RamTerm");
  glfwWindowHintString(GLFW_WAYLAND_APP_ID, "RamTerm");

  handle_ = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);

  if (!handle_) {
    glfwTerminate();
    throw std::runtime_error("Falha ao criar janela.");
  }

  glfwMakeContextCurrent(handle_);
  glfwSwapInterval(1);
  glfwFocusWindow(handle_);
  glfwSetWindowUserPointer(handle_, this);
  glfwSetKeyCallback(handle_, Window::keyCallback);
  glfwSetCharCallback(handle_, Window::charCallback);
  glfwSetScrollCallback(handle_, Window::scrollCallback);
  glfwSetFramebufferSizeCallback(handle_, Window::framebufferSizeCallback);
  glfwGetFramebufferSize(handle_, &width_, &height_);
}

Window::~Window() {
  if (handle_) {
    glfwDestroyWindow(handle_);
  }

  glfwTerminate();
}

bool Window::shouldClose() const { return glfwWindowShouldClose(handle_); }

void Window::pollEvents() const { glfwPollEvents(); }

void Window::waitEventsTimeout(double timeoutSeconds) const {
  if (handle_) glfwWaitEventsTimeout(timeoutSeconds);
}

void Window::swapBuffers() const { glfwSwapBuffers(handle_); }

int Window::getWidth() const {
  if (handle_) {
    glfwGetFramebufferSize(handle_, &width_, &height_);
  }
  return width_;
}

int Window::getHeight() const {
  if (handle_) {
    glfwGetFramebufferSize(handle_, &width_, &height_);
  }
  return height_;
}

bool Window::isIconified() const {
  return handle_ && glfwGetWindowAttrib(handle_, GLFW_ICONIFIED);
}

std::string Window::consumeInputBuffer() {
  std::string data = inputBuffer_;
  inputBuffer_.clear();
  return data;
}

double Window::consumeScrollDelta() {
  const double value = scrollDelta_;
  scrollDelta_ = 0.0;
  return value;
}

void Window::keyCallback(GLFWwindow *window, int key, int scancode, int action,
                         int mods) {
  (void)scancode;
  (void)mods;

  auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));

  if (!self) {
    return;
  }

  if (action != GLFW_PRESS && action != GLFW_REPEAT) {
    return;
  }

  if (mods & GLFW_MOD_CONTROL) {
    if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z) {
      self->inputBuffer_ += static_cast<char>(key - GLFW_KEY_A + 1);
      return;
    }
    if (key >= GLFW_KEY_LEFT_BRACKET && key <= GLFW_KEY_RIGHT_BRACKET) {
      if (key == GLFW_KEY_LEFT_BRACKET) {
        self->inputBuffer_ += '\x1b';
      }
      return;
    }
    if (key == GLFW_KEY_ESCAPE) {
      self->inputBuffer_ += '\x1b';
      return;
    }
  }

  switch (key) {
  case GLFW_KEY_ESCAPE:
    self->inputBuffer_ += '\x1b';
    break;
  case GLFW_KEY_ENTER:
  case GLFW_KEY_KP_ENTER:
    self->inputBuffer_ += '\r';
    break;
  case GLFW_KEY_BACKSPACE:
    self->inputBuffer_ += '\x7f';
    break;
  case GLFW_KEY_TAB:
    self->inputBuffer_ += '\t';
    break;
  case GLFW_KEY_UP:
    self->inputBuffer_ += "\x1b[A";
    break;
  case GLFW_KEY_DOWN:
    self->inputBuffer_ += "\x1b[B";
    break;
  case GLFW_KEY_RIGHT:
    self->inputBuffer_ += "\x1b[C";
    break;
  case GLFW_KEY_LEFT:
    self->inputBuffer_ += "\x1b[D";
    break;
  case GLFW_KEY_HOME:
    self->inputBuffer_ += "\x1b[H";
    break;
  case GLFW_KEY_END:
    self->inputBuffer_ += "\x1b[F";
    break;
  case GLFW_KEY_PAGE_UP:
    self->inputBuffer_ += "\x1b[5~";
    break;
  case GLFW_KEY_PAGE_DOWN:
    self->inputBuffer_ += "\x1b[6~";
    break;
  case GLFW_KEY_INSERT:
    self->inputBuffer_ += "\x1b[2~";
    break;
  case GLFW_KEY_DELETE:
    self->inputBuffer_ += "\x1b[3~";
    break;
  default:
    break;
  }
}

static void appendUtf8(std::string& out, unsigned int codepoint) {
  if (codepoint <= 0x7F) {
    out += static_cast<char>(codepoint);
    return;
  }
  if (codepoint <= 0x7FF) {
    out += static_cast<char>(0xC0u | (codepoint >> 6));
    out += static_cast<char>(0x80u | (codepoint & 0x3Fu));
    return;
  }
  if (codepoint <= 0xFFFF) {
    out += static_cast<char>(0xE0u | (codepoint >> 12));
    out += static_cast<char>(0x80u | ((codepoint >> 6) & 0x3Fu));
    out += static_cast<char>(0x80u | (codepoint & 0x3Fu));
    return;
  }
  if (codepoint <= 0x10FFFF) {
    out += static_cast<char>(0xF0u | (codepoint >> 18));
    out += static_cast<char>(0x80u | ((codepoint >> 12) & 0x3Fu));
    out += static_cast<char>(0x80u | ((codepoint >> 6) & 0x3Fu));
    out += static_cast<char>(0x80u | (codepoint & 0x3Fu));
  }
}

void Window::charCallback(GLFWwindow *window, unsigned int codepoint) {
  auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
  if (!self) return;
  appendUtf8(self->inputBuffer_, codepoint);
}

void Window::scrollCallback(GLFWwindow *window, double xoffset,
                            double yoffset) {
  (void)xoffset;
  auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));

  if (!self) {
    return;
  }

  self->scrollDelta_ += yoffset;
}

void Window::framebufferSizeCallback(GLFWwindow *window, int width,
                                     int height) {
  auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
  if (!self) { return; }
  self->width_ = width;
  self->height_ = height;
}