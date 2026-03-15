#pragma once

#include <cstdint>
#include <vector>
#include <algorithm>

/** Máximo de codepoints por célula (base + combinadores), compatível com libvterm. */
constexpr int kMaxCharsPerCell = 6;

struct TermColor {
  float r = 1.0f;
  float g = 1.0f;
  float b = 1.0f;
  float a = 1.0f;
};

struct TermCell {
  /** Primeiro codepoint (compatibilidade). */
  uint32_t codepoint = static_cast<uint32_t>(' ');
  /** Todos os codepoints da célula (base + combinadores); nchars indica quantos são válidos. */
  uint32_t codepoints[kMaxCharsPerCell] = {};
  int nchars = 0;
  /** Largura em células (1 = normal, 2 = caractere largo). Blocos U+2580..U+259F são forçados a 1. */
  int width = 1;
  TermColor foreground{1.0f, 1.0f, 1.0f, 1.0f};
  TermColor background{0.0f, 0.0f, 0.0f, 1.0f};
  bool bold = false;
  bool underline = false;
  bool inverse = false;
};

struct TermCursor {
  int row = 0;
  int column = 0;
  bool visible = true;
};

class VTermScreenBuffer {
public:
  VTermScreenBuffer();
  VTermScreenBuffer(int rows, int columns);

  void resize(int rows, int columns);
  void clear();

  int rows() const;
  int columns() const;

  const TermCell& cell(int row, int column) const;
  TermCell& cell(int row, int column);

  void setCell(int row, int column, const TermCell& value);

  void setCursor(const TermCursor& cursor);
  const TermCursor& cursor() const;

private:
  int rows_ = 0;
  int columns_ = 0;
  std::vector<TermCell> cells_;
  TermCursor cursor_;

  int index(int row, int column) const;
  bool valid(int row, int column) const;
};