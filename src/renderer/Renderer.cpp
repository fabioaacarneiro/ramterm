#include "renderer/Renderer.hpp"

#include <GL/gl.h>
#include <algorithm>
#include <cmath>
/* RamTerm renderer - cor do texto vem de theme.font do config.yaml */

Renderer::Renderer(FontManager& fontManager)
    : fontManager_(fontManager) {}

void Renderer::setViewport(int width, int height) {
  viewportWidth_ = std::max(1, width);
  viewportHeight_ = std::max(1, height);

  glViewport(0, 0, viewportWidth_, viewportHeight_);
}

void Renderer::setMetrics(const Metrics& metrics) {
  metrics_ = metrics;
}

/** Recebe cores em 0–255 (config theme.background). */
void Renderer::setClearColor(float r, float g, float b, float a) {
  if (r + g + b < 13.f) {
    r = 13.f;
    g = 13.f;
    b = 15.f;
  }
  clearColor_[0] = r;
  clearColor_[1] = g;
  clearColor_[2] = b;
  clearColor_[3] = (a <= 0.f) ? 255.f : a;
}

/** Recebe cores em 0–255 (config theme.font_color). */
void Renderer::setThemeForeground(float r, float g, float b, float a) {
  const float sum = r + g + b;
  if (sum < 51.f) {
    r = g = b = 255.f;
    a = 255.f;
  }
  themeForeground_[0] = r;
  themeForeground_[1] = g;
  themeForeground_[2] = b;
  themeForeground_[3] = (a <= 0.f) ? 255.f : a;
}

int Renderer::columnsForWidth(int width) const {
  return std::max(1, static_cast<int>(width / metrics_.cellWidth));
}

int Renderer::rowsForHeight(int height) const {
  return std::max(1, static_cast<int>(height / metrics_.cellHeight));
}

void Renderer::setScrollState(int scrollbackLines, int scrollOffset) {
  scrollbackLines_ = std::max(0, scrollbackLines);
  scrollOffset_ = std::max(0, scrollOffset);
}

void Renderer::drawScrollbar() {
  if (scrollbackLines_ <= 0) return;
  const int visibleRows = rowsForHeight(viewportHeight_);
  const int totalLines = scrollbackLines_ + visibleRows;
  if (totalLines <= 0) return;
  const float h = static_cast<float>(viewportHeight_);
  const float thumbHeight = (static_cast<float>(visibleRows) / static_cast<float>(totalLines)) * h;
  const float thumbTop = (static_cast<float>(scrollOffset_) / static_cast<float>(totalLines)) * h;
  const float x = static_cast<float>(viewportWidth_) - kScrollbarWidth;
  TermColor track;
  track.r = track.g = track.b = 0.25f;
  track.a = 1.f;
  drawQuad(x, 0.f, kScrollbarWidth, h, track);
  TermColor thumb;
  thumb.r = themeForeground_[0] / 255.f;
  thumb.g = themeForeground_[1] / 255.f;
  thumb.b = themeForeground_[2] / 255.f;
  thumb.a = 0.5f;
  drawQuad(x, thumbTop, kScrollbarWidth, thumbHeight, thumb);
}

void Renderer::setup2DProjection() const {
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(
      0.0,
      static_cast<double>(viewportWidth_),
      static_cast<double>(viewportHeight_),
      0.0,
      -1.0,
      1.0);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

void Renderer::drawQuad(float x, float y, float width, float height, const TermColor& color) {
  glColor4f(color.r, color.g, color.b, color.a);

  glBegin(GL_QUADS);
  glVertex2f(x, y);
  glVertex2f(x + width, y);
  glVertex2f(x + width, y + height);
  glVertex2f(x, y + height);
  glEnd();
}

void Renderer::drawCellBackground(float x, float y, float width, float height, const TermColor& color) {
  drawQuad(x, y, width, height, color);
}

void Renderer::drawGlyph(uint32_t codepoint, float x, float baselineY, const TermColor& color, bool bold) {
  const float r = std::min(255.f, std::max(0.f, color.r * 255.f));
  const float g = std::min(255.f, std::max(0.f, color.g * 255.f));
  const float b = std::min(255.f, std::max(0.f, color.b * 255.f));
  const float a = (color.a <= 0.f) ? 255.f : std::min(255.f, color.a * 255.f);
  const char32_t glyphText[2] = {
      static_cast<char32_t>(codepoint),
      U'\0'
  };
  fontManager_.drawText(glyphText, x, baselineY, bold, r, g, b, a);
}

void Renderer::drawCursor(float x, float y, float width, float height) {
  TermColor cursorColor;
  cursorColor.r = themeForeground_[0] / 255.f;
  cursorColor.g = themeForeground_[1] / 255.f;
  cursorColor.b = themeForeground_[2] / 255.f;
  cursorColor.a = themeForeground_[3] / 255.f;
  drawQuad(x, y, width, height, cursorColor);
}

void Renderer::render(const VTermScreenBuffer& buffer) {
  glViewport(0, 0, viewportWidth_, viewportHeight_);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  setup2DProjection();

  glClearColor(clearColor_[0] / 255.f, clearColor_[1] / 255.f, clearColor_[2] / 255.f, clearColor_[3] / 255.f);
  glClear(GL_COLOR_BUFFER_BIT);

  TermColor themeBg;
  themeBg.r = clearColor_[0] / 255.f;
  themeBg.g = clearColor_[1] / 255.f;
  themeBg.b = clearColor_[2] / 255.f;
  themeBg.a = clearColor_[3] / 255.f;
  drawQuad(0.f, 0.f, static_cast<float>(viewportWidth_), static_cast<float>(viewportHeight_), themeBg);

  const float originX = 0.0f;
  const float originY = 0.0f;

  const int rows = buffer.rows();
  const int cols = buffer.columns();

  for (int row = 0; row < rows; ++row) {
    float blockPenX = originX;
    for (int col = 0; col < cols; ++col) {
      const TermCell& cell = buffer.cell(row, col);

      const float x = std::floor(originX + static_cast<float>(col) * metrics_.cellWidth + 0.5f);
      const float y = std::floor(originY + static_cast<float>(row) * metrics_.cellHeight + 0.5f);

      const float bgLum = cell.background.r + cell.background.g + cell.background.b;
      const TermColor& bg = (bgLum < 0.2f) ? themeBg : cell.background;
      drawCellBackground(x, y, metrics_.cellWidth, metrics_.cellHeight, bg);

      const bool isSecondColumnOfWide = (col > 0 && buffer.cell(row, col - 1).width == 2);
      if (!isSecondColumnOfWide && cell.nchars > 0) {
        const float baselineY = std::floor(y + metrics_.ascent + 0.5f);
        const bool isBlockChar = (cell.codepoint >= 0x2580u && cell.codepoint <= 0x259Fu);
        const int advance = fontManager_.getGlyphAdvance(cell.codepoint);
        const float drawX = (isBlockChar && advance > 0) ? blockPenX : x;
        for (int i = 0; i < cell.nchars; ++i) {
          const uint32_t cp = cell.codepoints[i];
          if (cp != 0 && cp != static_cast<uint32_t>(' ')) {
            drawGlyph(cp, drawX, baselineY, cell.foreground, cell.bold);
          }
        }
        if (isBlockChar && advance > 0) {
          blockPenX += static_cast<float>(advance);
        } else {
          blockPenX = originX + static_cast<float>(col + 1) * metrics_.cellWidth;
        }
      } else {
        blockPenX = originX + static_cast<float>(col + 1) * metrics_.cellWidth;
      }
    }
  }

  const TermCursor& cursor = buffer.cursor();
  const float cursorX = originX + static_cast<float>(cursor.column) * metrics_.cellWidth;
  const float cursorY = originY + static_cast<float>(cursor.row) * metrics_.cellHeight;
  smoothCursorX_ = cursorX;
  smoothCursorY_ = cursorY;

  if (cursor.visible) {
    const float cursorWidth = std::max(1.0f, std::floor(metrics_.cellWidth * 0.08f));
    const float cursorHeight = metrics_.cellHeight;
    drawCursor(cursorX, cursorY, cursorWidth, cursorHeight);
  }

  drawScrollbar();
}