#pragma once

#include "font/FontManager.hpp"
#include "terminal/VTermScreenBuffer.hpp"

class Renderer {
public:
  struct Metrics {
    float cellWidth = 9.0f;
    float cellHeight = 18.0f;
    float ascent = 14.0f;
    /** Tamanho da fonte em pixels (config); usado para passo dos blocos U+2580–U+259F. */
    float fontSize = 12.0f;
  };

  explicit Renderer(FontManager& fontManager);

  void setViewport(int width, int height);
  void setMetrics(const Metrics& metrics);
  void setClearColor(float r, float g, float b, float a);
  void setThemeForeground(float r, float g, float b, float a);
  /** Cor do destaque da seleção (0–255; alpha ex.: 89 para ~0.35). */
  void setSelectionColor(float r, float g, float b, float a);

  int columnsForWidth(int width) const;
  int rowsForHeight(int height) const;

  void setScrollState(int scrollbackLines, int scrollOffset);
  /** Seleção: (r0,c0) até (r1,c1) inclusive; -1 = sem seleção. */
  void setSelection(int r0, int c0, int r1, int c1);
  void render(const VTermScreenBuffer& buffer);

private:
  void setup2DProjection() const;
  void drawQuad(float x, float y, float width, float height, const TermColor& color);
  void drawRoundedQuad(float x, float y, float width, float height, float radius, const TermColor& color);
  void drawRoundedQuadBorder(float x, float y, float width, float height, float radius, const TermColor& color);
  void drawCellBackground(float x, float y, float width, float height, const TermColor& color);
  void drawGlyph(uint32_t codepoint, float x, float baselineY, const TermColor& color, bool bold);
  void drawCursor(float x, float y, float width, float height);
  void drawScrollbar();

private:
  FontManager& fontManager_;
  int viewportWidth_ = 1;
  int viewportHeight_ = 1;
  Metrics metrics_{};
  /** Cores em 0–255 (como no config). Convertidas para 0–1 só em glClearColor. */
  float clearColor_[4] = {0.f, 0.f, 0.f, 255.f};
  /** Cor do texto em 0–255. Sobrescrito por setThemeForeground(config.theme.font). */
  float themeForeground_[4] = {255.f, 255.f, 255.f, 255.f};
  /** Cor da seleção em 0–255 (theme.selection). */
  float selectionColor_[4] = {255.f, 255.f, 255.f, 89.f};
  float smoothCursorX_ = 0.f;
  float smoothCursorY_ = 0.f;
  int scrollbackLines_ = 0;
  int scrollOffset_ = 0;
  int selStartRow_ = -1;
  int selStartCol_ = -1;
  int selEndRow_ = -1;
  int selEndCol_ = -1;
  static constexpr float kScrollbarWidth = 6.f;
};