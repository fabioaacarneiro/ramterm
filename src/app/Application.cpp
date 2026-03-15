#include "app/Application.hpp"

#include <array>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

Application::Application()
    : config_(Config::loadFromSearchPaths({"config/config", "../config/config"})),
      window_(config_.window.width, config_.window.height, config_.window.title),
      fontManager_(FontConfig{
          .path = config_.font.path,
          .size = config_.font.size
      }),
      renderer_(fontManager_),
      terminal_(24, 80) {
  Renderer::Metrics metrics;
  float cw = static_cast<float>(fontManager_.getGlyphAdvance('M'));
  float ch = static_cast<float>(fontManager_.getLineHeight());
  float asc = static_cast<float>(fontManager_.getAscender());
  if (cw < 4.0f) cw = static_cast<float>(config_.font.size) * 0.6f;
  if (ch < 4.0f) ch = static_cast<float>(config_.font.size) * 1.2f;
  if (asc < 1.0f) asc = ch * 0.8f;
  metrics.cellWidth = cw;
  metrics.cellHeight = ch;
  metrics.ascent = asc;
  metrics.fontSize = static_cast<float>(fontManager_.getPixelSize());
  renderer_.setMetrics(metrics);

  const ThemeConfig& effectiveTheme = config_.theme.use_default_theme
      ? Config::getTangoDarkTheme()
      : config_.theme;
  renderer_.setClearColor(
      effectiveTheme.background.r,
      effectiveTheme.background.g,
      effectiveTheme.background.b,
      effectiveTheme.background.a
  );
  renderer_.setThemeForeground(
      effectiveTheme.font.r,
      effectiveTheme.font.g,
      effectiveTheme.font.b,
      effectiveTheme.font.a
  );
  terminal_.setTheme(effectiveTheme);

  std::cerr << "[RamTerm] config.yaml: janela " << config_.window.width << "x" << config_.window.height
            << ", tema: " << (config_.theme.use_default_theme ? "Tango Dark (padrao)" : "personalizado")
            << ", fonte: " << fontManager_.getFontPath()
            << ", cor texto R=" << static_cast<int>(effectiveTheme.font.r) << " G=" << static_cast<int>(effectiveTheme.font.g) << " B=" << static_cast<int>(effectiveTheme.font.b) << " (0-255)";
  if (effectiveTheme.font.r >= 250.f && effectiveTheme.font.g >= 250.f && effectiveTheme.font.b >= 250.f) {
    std::cerr << " (branco)";
  } else if (effectiveTheme.font.r + effectiveTheme.font.g + effectiveTheme.font.b < 51.f) {
    std::cerr << " (sera forrado para claro)";
  }
  std::cerr << "\n";

  if (!pty_.spawn(config_.shell, 24, 80)) {
    const char* err = std::strerror(errno);
    throw std::runtime_error(std::string("Falha ao iniciar o shell '") + config_.shell + "': " + (err ? err : "?"));
  }
}

void Application::run() {
  onResize(window_.getWidth(), window_.getHeight());
  bool needRender = true;

  while (!window_.shouldClose() && pty_.isRunning()) {
    window_.waitEventsTimeout(0.016);

    const int w = window_.getWidth();
    const int h = window_.getHeight();
    const bool sizeChanged = (w != windowWidth_ || h != windowHeight_);
    const bool validSize = (w >= 10 && h >= 10);
    if (sizeChanged && validSize && !window_.isIconified()) {
      onResize(w, h);
      needRender = true;
    }

    std::string input = window_.consumeInputBuffer();
    if (!input.empty()) {
      pty_.write(input.data(), input.size());
      needRender = true;
    }

    if (pumpPty()) {
      needRender = true;
    }

    const double scrollDelta = window_.consumeScrollDelta();
    if (scrollDelta != 0.0 && !terminal_.isUsingAltScreen()) {
      terminal_.addScrollOffset(static_cast<int>(scrollDelta));
      needRender = true;
    }

    if (needRender) {
      const int sbLines = terminal_.isUsingAltScreen() ? 0 : terminal_.getScrollbackLines();
      const int sbOffset = terminal_.isUsingAltScreen() ? 0 : terminal_.getScrollOffset();
      renderer_.setScrollState(sbLines, sbOffset);
      renderer_.render(terminal_.getActiveBuffer());
      window_.swapBuffers();
      needRender = false;
    }
  }
}

void Application::onResize(int pixelWidth, int pixelHeight) {
  windowWidth_ = pixelWidth;
  windowHeight_ = pixelHeight;

  renderer_.setViewport(windowWidth_, windowHeight_);

  const int cols = renderer_.columnsForWidth(windowWidth_);
  const int rows = renderer_.rowsForHeight(windowHeight_);

  pty_.resize(rows, cols);
  terminal_.resize(rows, cols);

  pumpPty();
}

bool Application::pumpPty() {
  std::array<char, 8192> buffer{};
  bool hadData = false;

  while (true) {
    const ssize_t bytes = pty_.read(buffer.data(), buffer.size());
    if (bytes <= 0) {
      break;
    }
    hadData = true;
    terminal_.feed(std::string(buffer.data(), static_cast<size_t>(bytes)));
  }
  return hadData;
}
