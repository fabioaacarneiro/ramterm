#include "Window.hpp"
#include <cstdlib>
#include <cstdio>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Window::Window(int width, int height, const std::string& title, const std::string& iconPath)
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
  glfwSetMouseButtonCallback(handle_, Window::mouseButtonCallback);
  glfwSetCursorPosCallback(handle_, Window::cursorPosCallback);
  glfwSetFramebufferSizeCallback(handle_, Window::framebufferSizeCallback);
  glfwSetWindowFocusCallback(handle_, Window::windowFocusCallback);
  glfwGetFramebufferSize(handle_, &width_, &height_);

  auto tryLoadIcon = [this](const char* path) -> bool {
    if (!path || !path[0]) return false;
    int iconW = 0, iconH = 0, channels = 0;
    unsigned char* pixels = stbi_load(path, &iconW, &iconH, &channels, STBI_rgb_alpha);
    if (!pixels || iconW <= 0 || iconH <= 0) {
      if (pixels) stbi_image_free(pixels);
      return false;
    }
    const int maxIcon = 64;
    int outW = iconW, outH = iconH;
    if (iconW > maxIcon || iconH > maxIcon) {
      if (iconW > iconH) {
        outW = maxIcon;
        outH = (iconH * maxIcon) / iconW;
        if (outH < 1) outH = 1;
      } else {
        outH = maxIcon;
        outW = (iconW * maxIcon) / iconH;
        if (outW < 1) outW = 1;
      }
    }
    unsigned char* outPixels = pixels;
    if (outW != iconW || outH != iconH) {
      outPixels = static_cast<unsigned char*>(malloc(static_cast<size_t>(outW * outH * 4)));
      if (!outPixels) { stbi_image_free(pixels); return false; }
      for (int y = 0; y < outH; ++y)
        for (int x = 0; x < outW; ++x) {
          int sx = (x * iconW) / outW, sy = (y * iconH) / outH;
          size_t srcIdx = static_cast<size_t>((sy * iconW + sx) * 4);
          size_t dstIdx = static_cast<size_t>((y * outW + x) * 4);
          outPixels[dstIdx] = pixels[srcIdx];
          outPixels[dstIdx + 1] = pixels[srcIdx + 1];
          outPixels[dstIdx + 2] = pixels[srcIdx + 2];
          outPixels[dstIdx + 3] = pixels[srcIdx + 3];
        }
      stbi_image_free(pixels);
    }
    GLFWimage icon;
    icon.width = outW;
    icon.height = outH;
    icon.pixels = outPixels;
    glfwSetWindowIcon(handle_, 1, &icon);
    if (outPixels != pixels) free(outPixels);
    else stbi_image_free(pixels);
    return true;
  };

  if (!iconPath.empty() && tryLoadIcon(iconPath.c_str()))
    (void)0;
  else if (!tryLoadIcon("assets/ramterm-logo.png") && !tryLoadIcon("../assets/ramterm-logo.png"))
    (void)0;
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

int Window::getWindowWidth() const {
  if (!handle_) return 0;
  int w = 0, h = 0;
  glfwGetWindowSize(handle_, &w, &h);
  return w > 0 ? w : 1;
}

int Window::getWindowHeight() const {
  if (!handle_) return 0;
  int w = 0, h = 0;
  glfwGetWindowSize(handle_, &w, &h);
  return h > 0 ? h : 1;
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

void Window::setClipboardString(const std::string& text) const {
  if (handle_) glfwSetClipboardString(handle_, text.c_str());
}

std::string Window::getClipboardString() const {
  if (!handle_) return {};
  const char* p = glfwGetClipboardString(handle_);
  return p ? std::string(p) : std::string();
}

void Window::keyCallback(GLFWwindow *window, int key, int scancode, int action,
                         int mods) {
  (void)scancode;

  auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));

  if (!self) {
    return;
  }

  if (action == GLFW_PRESS) {
    if (self->fontZoomCallback_) {
      const bool ctrl = (mods & GLFW_MOD_CONTROL) != 0;
      if (ctrl && (key == GLFW_KEY_EQUAL || key == GLFW_KEY_KP_ADD)) {
        self->fontZoomCallback_(1);
        return;
      }
      if (ctrl && (key == GLFW_KEY_MINUS || key == GLFW_KEY_KP_SUBTRACT)) {
        self->fontZoomCallback_(-1);
        return;
      }
    }
#ifdef __APPLE__
    if (key == GLFW_KEY_C && (mods & GLFW_MOD_SUPER)) {
      if (self->copyCallback_) self->copyCallback_();
      return;
    }
    if (key == GLFW_KEY_V && (mods & GLFW_MOD_SUPER)) {
      if (self->pasteCallback_) self->pasteCallback_();
      return;
    }
#else
    if (key == GLFW_KEY_C && (mods & GLFW_MOD_CONTROL) && (mods & GLFW_MOD_SHIFT)) {
      if (self->copyCallback_) self->copyCallback_();
      return;
    }
    if (key == GLFW_KEY_V && (mods & GLFW_MOD_CONTROL) && (mods & GLFW_MOD_SHIFT)) {
      if (self->pasteCallback_) self->pasteCallback_();
      return;
    }
#endif
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

void Window::mouseButtonCallback(GLFWwindow *window, int button, int action, int mods) {
  (void)mods;
  auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
  if (!self || !self->mouseButtonCallback_) return;
  double x, y;
  glfwGetCursorPos(window, &x, &y);
  self->mouseButtonCallback_(x, y, button, action);
}

void Window::cursorPosCallback(GLFWwindow *window, double x, double y) {
  auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
  if (!self || !self->cursorPosCallback_) return;
  self->cursorPosCallback_(x, y);
}

void Window::framebufferSizeCallback(GLFWwindow *window, int width,
                                     int height) {
  auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
  if (!self) { return; }
  self->width_ = width;
  self->height_ = height;
}

void Window::windowFocusCallback(GLFWwindow *window, int focused) {
  auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
  if (!self || focused != GLFW_TRUE) return;
  if (self->windowFocusCallback_) self->windowFocusCallback_();
}