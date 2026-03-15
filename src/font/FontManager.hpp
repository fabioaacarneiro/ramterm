#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

struct FontConfig {
  std::string path;
  int size = 16;
};

struct GlyphBitmap {
  int width = 0;
  int height = 0;
  int bearingX = 0;
  int bearingY = 0;
  int advance = 0;
  std::vector<unsigned char> pixels;
};

class FontManager {
public:
  explicit FontManager(const FontConfig &config);
  ~FontManager();

  FontManager(const FontManager &) = delete;
  FontManager &operator=(const FontManager &) = delete;

  int getPixelSize() const;
  const std::string &getFontPath() const;

  int getLineHeight() const;
  int getAscender() const;
  int getDescender() const;
  int getGlyphAdvance(uint32_t codepoint) const;

  GlyphBitmap loadGlyph(uint32_t codepoint) const;

  void drawText(const char *text, float x, float baselineY, bool bold, float colorR, float colorG, float colorB, float colorA) const;
  void drawText(const char32_t *text, float x, float baselineY, bool bold, float colorR, float colorG, float colorB, float colorA) const;

private:
  void drawGlyphBitmap(const GlyphBitmap &glyph, float x, float baselineY, bool bold, float colorR, float colorG, float colorB, float colorA) const;

private:
  FT_Library library_ = nullptr;
  FT_Face face_ = nullptr;
  FontConfig config_;
  mutable std::unordered_map<uint32_t, GlyphBitmap> glyphCache_;
};