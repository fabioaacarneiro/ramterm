#pragma once

#include <cstdint>

struct TermColor {
  float r = 1.0f;
  float g = 1.0f;
  float b = 1.0f;
  float a = 1.0f;
};

struct TermCell {
  uint32_t codepoint = ' ';
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