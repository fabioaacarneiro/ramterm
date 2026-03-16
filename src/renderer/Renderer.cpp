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

/** Recebe cores em 0–255 (config theme.selection). */
void Renderer::setSelectionColor(float r, float g, float b, float a) {
  selectionColor_[0] = std::max(0.f, std::min(255.f, r));
  selectionColor_[1] = std::max(0.f, std::min(255.f, g));
  selectionColor_[2] = std::max(0.f, std::min(255.f, b));
  selectionColor_[3] = (a <= 0.f) ? 89.f : std::max(0.f, std::min(255.f, a));
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

void Renderer::setSelection(int r0, int c0, int r1, int c1) {
  selStartRow_ = r0;
  selStartCol_ = c0;
  selEndRow_ = r1;
  selEndCol_ = c1;
}

void Renderer::drawScrollbar() {
  /* Barra de rolagem desativada inicialmente; scroll com roda do mouse continua funcionando. */
  (void)scrollbackLines_;
  (void)scrollOffset_;
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

namespace {
constexpr float kSelRadius = 2.5f;
constexpr float kSelPadding = 1.f;
constexpr float kSelBorderInset = 0.5f;  /* borda desenhada para dentro para parecer mais fina */
constexpr int kArcSegments = 6;
}  // namespace

void Renderer::drawRoundedQuad(float x, float y, float width, float height, float radius, const TermColor& color) {
  const float r = std::min({radius, width * 0.5f, height * 0.5f});
  if (r <= 0.f) {
    drawQuad(x, y, width, height, color);
    return;
  }
  glColor4f(color.r, color.g, color.b, color.a);
  const float cx = x + width;
  const float cy = y + height;

  glBegin(GL_QUADS);
  glVertex2f(x + r, y);
  glVertex2f(cx - r, y);
  glVertex2f(cx - r, y + r);
  glVertex2f(x + r, y + r);

  glVertex2f(x + r, cy - r);
  glVertex2f(cx - r, cy - r);
  glVertex2f(cx - r, cy);
  glVertex2f(x + r, cy);

  glVertex2f(x, y + r);
  glVertex2f(x + r, y + r);
  glVertex2f(x + r, cy - r);
  glVertex2f(x, cy - r);

  glVertex2f(cx - r, y + r);
  glVertex2f(cx, y + r);
  glVertex2f(cx, cy - r);
  glVertex2f(cx - r, cy - r);
  glEnd();

  const float pi = static_cast<float>(M_PI);
  /* Top-left: arc 90° → 180° */
  for (int i = 0; i < kArcSegments; ++i) {
    const float a0 = pi * 0.5f + pi * 0.5f * (static_cast<float>(i) / static_cast<float>(kArcSegments));
    const float a1 = pi * 0.5f + pi * 0.5f * (static_cast<float>(i + 1) / static_cast<float>(kArcSegments));
    glBegin(GL_TRIANGLES);
    glVertex2f(x + r, y + r);
    glVertex2f(x + r + r * std::cos(a0), y + r - r * std::sin(a0));
    glVertex2f(x + r + r * std::cos(a1), y + r - r * std::sin(a1));
    glEnd();
  }
  /* Top-right: arc 0° → 90° */
  for (int i = 0; i < kArcSegments; ++i) {
    const float a0 = pi * 0.5f * (static_cast<float>(i) / static_cast<float>(kArcSegments));
    const float a1 = pi * 0.5f * (static_cast<float>(i + 1) / static_cast<float>(kArcSegments));
    glBegin(GL_TRIANGLES);
    glVertex2f(cx - r, y + r);
    glVertex2f(cx - r + r * std::cos(a0), y + r - r * std::sin(a0));
    glVertex2f(cx - r + r * std::cos(a1), y + r - r * std::sin(a1));
    glEnd();
  }
  /* Bottom-right: arc 270° → 360° */
  for (int i = 0; i < kArcSegments; ++i) {
    const float a0 = pi * 1.5f + pi * 0.5f * (static_cast<float>(i) / static_cast<float>(kArcSegments));
    const float a1 = pi * 1.5f + pi * 0.5f * (static_cast<float>(i + 1) / static_cast<float>(kArcSegments));
    glBegin(GL_TRIANGLES);
    glVertex2f(cx - r, cy - r);
    glVertex2f(cx - r + r * std::cos(a0), cy - r + r * std::sin(a0));
    glVertex2f(cx - r + r * std::cos(a1), cy - r + r * std::sin(a1));
    glEnd();
  }
  /* Bottom-left: arc 180° → 270° */
  for (int i = 0; i < kArcSegments; ++i) {
    const float a0 = pi + pi * 0.5f * (static_cast<float>(i) / static_cast<float>(kArcSegments));
    const float a1 = pi + pi * 0.5f * (static_cast<float>(i + 1) / static_cast<float>(kArcSegments));
    glBegin(GL_TRIANGLES);
    glVertex2f(x + r, cy - r);
    glVertex2f(x + r + r * std::cos(a0), cy - r + r * std::sin(a0));
    glVertex2f(x + r + r * std::cos(a1), cy - r + r * std::sin(a1));
    glEnd();
  }
}

void Renderer::drawRoundedQuadBorder(float x, float y, float width, float height, float radius, const TermColor& color) {
  const float t = kSelBorderInset;
  x += t;
  y += t;
  width -= 2.f * t;
  height -= 2.f * t;
  const float r = std::min({radius - t, width * 0.5f, height * 0.5f});
  if (r <= 0.f) return;
  glColor4f(color.r, color.g, color.b, color.a);
  glLineWidth(1.f);
  const float cx = x + width;
  const float cy = y + height;
  const float pi = static_cast<float>(M_PI);

  glBegin(GL_LINE_LOOP);
  glVertex2f(x + r, y);
  glVertex2f(cx - r, y);
  for (int i = 1; i <= kArcSegments; ++i) {
    const float a = pi * 0.5f * (1.f - static_cast<float>(i) / static_cast<float>(kArcSegments));
    glVertex2f(cx - r + r * std::cos(a), y + r - r * std::sin(a));
  }
  glVertex2f(cx, y + r);
  glVertex2f(cx, cy - r);
  for (int i = 1; i <= kArcSegments; ++i) {
    const float a = pi * 0.5f * (static_cast<float>(i) / static_cast<float>(kArcSegments));
    glVertex2f(cx - r + r * std::cos(a), cy - r + r * std::sin(a));
  }
  glVertex2f(cx - r, cy);
  glVertex2f(x + r, cy);
  for (int i = 1; i <= kArcSegments; ++i) {
    const float a = pi + pi * 0.5f * (static_cast<float>(i) / static_cast<float>(kArcSegments));
    glVertex2f(x + r + r * std::cos(a), cy - r + r * std::sin(a));
  }
  glVertex2f(x, cy - r);
  glVertex2f(x, y + r);
  for (int i = 1; i <= kArcSegments; ++i) {
    const float a = pi * 0.5f + pi * 0.5f * (static_cast<float>(i) / static_cast<float>(kArcSegments));
    glVertex2f(x + r + r * std::cos(a), y + r - r * std::sin(a));
  }
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

  int selR0 = selStartRow_, selC0 = selStartCol_, selR1 = selEndRow_, selC1 = selEndCol_;
  if (selR0 > selR1 || (selR0 == selR1 && selC0 > selC1)) {
    std::swap(selR0, selR1);
    std::swap(selC0, selC1);
  }
  const bool hasSel = (selR0 >= 0 && selC0 >= 0 && selR1 >= 0 && selC1 >= 0);

  int lastUsedRow = -1;
  if (hasSel) {
    for (int r = rows - 1; r >= 0; --r) {
      for (int c = 0; c < cols; ++c) {
        const TermCell& cell = buffer.cell(r, c);
        for (int i = 0; i < cell.nchars; ++i) {
          if (cell.codepoints[i] != 0 && cell.codepoints[i] != static_cast<uint32_t>(' ')) {
            lastUsedRow = r;
            goto done_last_used_row;
          }
        }
      }
    }
  done_last_used_row:;
    if (lastUsedRow >= 0 && selR1 > lastUsedRow) selR1 = lastUsedRow;
  }

  if (hasSel && selR0 == selR1) {
    int lastNonSpace = selC0;
    for (int c = cols - 1; c >= selC0; --c) {
      const TermCell& cell = buffer.cell(selR0, c);
      bool hasNonSpace = false;
      for (int i = 0; i < cell.nchars; ++i) {
        const uint32_t cp = cell.codepoints[i];
        if (cp != 0 && cp != static_cast<uint32_t>(' ')) { hasNonSpace = true; break; }
      }
      if (hasNonSpace) { lastNonSpace = c; break; }
    }
    selC1 = std::min(selC1, lastNonSpace);
  }

  for (int row = 0; row < rows; ++row) {
    int lastNonSpaceRow = -1;
    if (hasSel && row >= selR0 && row <= selR1) {
      for (int c = cols - 1; c >= 0; --c) {
        const TermCell& cell = buffer.cell(row, c);
        for (int i = 0; i < cell.nchars; ++i) {
          if (cell.codepoints[i] != 0 && cell.codepoints[i] != static_cast<uint32_t>(' ')) {
            lastNonSpaceRow = c;
            break;
          }
        }
        if (lastNonSpaceRow >= 0) break;
      }
    }

    float blockPenX = originX;
    for (int col = 0; col < cols; ++col) {
      const TermCell& cell = buffer.cell(row, col);
      const float x = std::floor(originX + static_cast<float>(col) * metrics_.cellWidth + 0.5f);
      const float y = std::floor(originY + static_cast<float>(row) * metrics_.cellHeight + 0.5f);

      const float bgLum = cell.background.r + cell.background.g + cell.background.b;
      const TermColor& bg = (bgLum < 0.2f) ? themeBg : cell.background;
      drawCellBackground(x, y, metrics_.cellWidth, metrics_.cellHeight, bg);

      bool inSel = false;
      if (hasSel && row <= lastUsedRow) {
        if (selR0 == selR1) {
          inSel = (row == selR0) && (col >= selC0 && col <= selC1);
        } else {
          if (lastNonSpaceRow >= 0) {
            if (row == selR0)
              inSel = (col >= selC0) && (col <= lastNonSpaceRow);
            else if (row == selR1)
              inSel = (col <= selC1) && (col <= lastNonSpaceRow);
            else if (row > selR0 && row < selR1)
              inSel = (col <= lastNonSpaceRow);
          }
        }
      }
      if (inSel) {
        TermColor selBg;
        selBg.r = selectionColor_[0] / 255.f;
        selBg.g = selectionColor_[1] / 255.f;
        selBg.b = selectionColor_[2] / 255.f;
        selBg.a = selectionColor_[3] / 255.f;
        drawQuad(x, y, metrics_.cellWidth, metrics_.cellHeight, selBg);
      }

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