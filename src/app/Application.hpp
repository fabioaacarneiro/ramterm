#pragma once

#include "config/Config.hpp"
#include "font/FontManager.hpp"
#include "pty/PtyProcess.hpp"
#include "renderer/Renderer.hpp"
#include "terminal/VTermEngine.hpp"
#include "window/Window.hpp"

class Application {
public:
  Application();
  void run();
  void onResize(int pixelWidth, int pixelHeight);
  /** Retorna true se leu algum dado do PTY (conteúdo do terminal mudou). */
  bool pumpPty();

private:
  AppConfig config_;
  Window window_;
  FontManager fontManager_;
  Renderer renderer_;
  PtyProcess pty_;
  VTermEngine terminal_;
  int windowWidth_ = 1280;
  int windowHeight_ = 720;
};