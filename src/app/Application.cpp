#include "app/Application.hpp"

#include <GLFW/glfw3.h>
#include <array>
#include <cerrno>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

Application::Application()
    : config_(Config::loadFromSearchPaths([]() {
        std::vector<std::string> paths = {"config/config", "../config/config"};
        for (const std::string& p : Config::getDefaultConfigSearchPaths()) paths.push_back(p);
        return paths;
      }())),
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
  renderer_.setSelectionColor(
      effectiveTheme.selection.r,
      effectiveTheme.selection.g,
      effectiveTheme.selection.b,
      effectiveTheme.selection.a
  );
  terminal_.setTheme(effectiveTheme);

  window_.setMouseButtonCallback([this](double x, double y, int button, int action) {
    if (button != GLFW_MOUSE_BUTTON_LEFT) return;
    int row, col;
    pixelToCellFromCursor(x, y, row, col);
    const int rows = terminal_.getRows();
    const int cols = terminal_.getColumns();
    if (row < 0 || row >= rows || col < 0 || col >= cols) return;
    if (!terminal_.isUsingAltScreen()) {
      if (action == GLFW_PRESS) {
        const double now = glfwGetTime();
        if (now - lastPressTime_ < 0.45 && row == lastPressRow_ && col == lastPressCol_) {
          lastClickCount_ = (lastClickCount_ >= 3) ? 3 : lastClickCount_ + 1;
        } else {
          lastClickCount_ = 1;
        }
        lastPressTime_ = now;
        lastPressRow_ = row;
        lastPressCol_ = col;

        selAnchorRow_ = row;
        selAnchorCol_ = col;
        lastDragRow_ = row;
        lastDragCol_ = col;
        selHasMoved_ = false;
        terminal_.setSelection(-1, -1, -1, -1);
        selecting_ = true;
        requestRender_ = true;
      } else if (action == GLFW_RELEASE) {
        if (selHasMoved_) {
          int row = lastDragRow_, col = lastDragCol_;
          snapSelectionEndToSameLineIfAccidental(x, row, col, cols, window_.getWindowWidth());
          terminal_.setSelection(selAnchorRow_, selAnchorCol_, row, col);
        } else {
          if (lastClickCount_ == 2) {
            int w0, w1;
            getWordBounds(selAnchorRow_, selAnchorCol_, w0, w1);
            terminal_.setSelection(selAnchorRow_, w0, selAnchorRow_, w1);
          } else if (lastClickCount_ == 3) {
            terminal_.setSelection(selAnchorRow_, 0, selAnchorRow_, cols - 1);
          }
        }
        selecting_ = false;
        requestRender_ = true;
      }
    }
  });

  window_.setCursorPosCallback([this](double x, double y) {
    if (!selecting_) return;
    selHasMoved_ = true;
    int row, col;
    pixelToCellFromCursor(x, y, row, col);
    const int cols = terminal_.getColumns();
    const int rows = terminal_.getRows();
    snapSelectionEndToSameLineIfAccidental(x, row, col, cols, window_.getWindowWidth());
    lastDragRow_ = std::max(0, std::min(rows - 1, row));
    lastDragCol_ = std::max(0, std::min(cols - 1, col));
    terminal_.setSelection(selAnchorRow_, selAnchorCol_, lastDragRow_, lastDragCol_);
    requestRender_ = true;
  });

  window_.setFontZoomCallback([this](int delta) {
    int sz = fontManager_.getPixelSize() + delta;
    if (sz < 6) sz = 6;
    if (sz > 256) sz = 256;
    fontManager_.setPixelSize(sz);
    applyFontMetricsAndResize();
    requestRender_ = true;
  });

  window_.setCopyCallback([this]() {
    std::string s = terminal_.getSelectedText();
    if (!s.empty()) window_.setClipboardString(s);
  });

  window_.setPasteCallback([this]() {
    std::string s = window_.getClipboardString();
    if (!s.empty()) {
      pty_.write(s.data(), s.size());
    }
  });

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

    if (requestRender_) {
      needRender = true;
      requestRender_ = false;
    }

    if (needRender) {
      const int sbLines = terminal_.isUsingAltScreen() ? 0 : terminal_.getScrollbackLines();
      const int sbOffset = terminal_.isUsingAltScreen() ? 0 : terminal_.getScrollOffset();
      renderer_.setScrollState(sbLines, sbOffset);
      int sr, sc, er, ec;
      terminal_.getSelection(sr, sc, er, ec);
      renderer_.setSelection(sr, sc, er, ec);
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

void Application::applyFontMetricsAndResize() {
  Renderer::Metrics metrics;
  float cw = static_cast<float>(fontManager_.getGlyphAdvance('M'));
  float ch = static_cast<float>(fontManager_.getLineHeight());
  float asc = static_cast<float>(fontManager_.getAscender());
  const int sz = fontManager_.getPixelSize();
  if (cw < 4.0f) cw = static_cast<float>(sz) * 0.6f;
  if (ch < 4.0f) ch = static_cast<float>(sz) * 1.2f;
  if (asc < 1.0f) asc = ch * 0.8f;
  metrics.cellWidth = cw;
  metrics.cellHeight = ch;
  metrics.ascent = asc;
  metrics.fontSize = static_cast<float>(sz);
  renderer_.setMetrics(metrics);
  onResize(windowWidth_, windowHeight_);
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

void Application::pixelToCell(double x, double y, int& row, int& col) const {
  if (windowHeight_ <= 0 || windowWidth_ <= 0) {
    row = 0;
    col = 0;
    return;
  }
  const int rows = terminal_.getRows();
  const int cols = terminal_.getColumns();
  row = static_cast<int>(y * static_cast<double>(rows) / static_cast<double>(windowHeight_));
  col = static_cast<int>(x * static_cast<double>(cols) / static_cast<double>(windowWidth_));
  row = std::max(0, std::min(rows - 1, row));
  col = std::max(0, std::min(cols - 1, col));
}

void Application::pixelToCellFromCursor(double x, double y, int& row, int& col) const {
  const int fbW = window_.getWidth();
  const int fbH = window_.getHeight();
  const int winW = window_.getWindowWidth();
  const int winH = window_.getWindowHeight();
  if (fbW <= 0 || fbH <= 0) {
    row = 0;
    col = 0;
    return;
  }
  double scaleX = (winW > 0) ? static_cast<double>(fbW) / static_cast<double>(winW) : 1.0;
  double scaleY = (winH > 0) ? static_cast<double>(fbH) / static_cast<double>(winH) : 1.0;
  const double fx = x * scaleX;
  const double fy = y * scaleY;
  const int rows = terminal_.getRows();
  const int cols = terminal_.getColumns();
  row = static_cast<int>(fy * static_cast<double>(rows) / static_cast<double>(fbH));
  col = static_cast<int>(fx * static_cast<double>(cols) / static_cast<double>(fbW));
  row = std::max(0, std::min(rows - 1, row));
  col = std::max(0, std::min(cols - 1, col));
}

int Application::columnFromX(double x, int cols, int pixelWidth) const {
  if (cols <= 0 || pixelWidth <= 0) return 0;
  int col = static_cast<int>(x * static_cast<double>(cols) / static_cast<double>(pixelWidth));
  return std::max(0, std::min(cols - 1, col));
}

void Application::snapSelectionEndToSameLineIfAccidental(double x, int& endRow, int& endCol, int cols, int pixelWidth) const {
  if (cols <= 0 || pixelWidth <= 0) return;
  if (endRow == selAnchorRow_ + 1 && endCol <= 2) {
    endRow = selAnchorRow_;
    endCol = columnFromX(x, cols, pixelWidth);
  }
}

static bool isWordChar(uint32_t cp) {
  return (cp >= '0' && cp <= '9') || (cp >= 'A' && cp <= 'Z') ||
         (cp >= 'a' && cp <= 'z') || cp == '_' || (cp > 127 && cp < 0x10000);
}

void Application::getWordBounds(int row, int col, int& startCol, int& endCol) const {
  const auto& buf = terminal_.getActiveBuffer();
  const int cols = terminal_.getColumns();
  if (row < 0 || row >= terminal_.getRows() || col < 0 || col >= cols) {
    startCol = endCol = col;
    return;
  }
  startCol = col;
  while (startCol > 0) {
    int prev = startCol - 1;
    if (prev > 0 && buf.cell(row, prev - 1).width >= 2) {
      startCol = prev - 1;
      continue;
    }
    if (!isWordChar(buf.cell(row, prev).codepoint)) break;
    startCol = prev;
  }
  endCol = col;
  while (endCol < cols - 1) {
    if (buf.cell(row, endCol).width >= 2) {
      endCol++;
      continue;
    }
    int next = endCol + 1;
    if (next >= cols) break;
    if (!isWordChar(buf.cell(row, next).codepoint)) break;
    endCol = next;
  }
}
