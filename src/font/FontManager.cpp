#include "font/FontManager.hpp"

#include <GL/gl.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <unordered_map>

FontManager::FontManager(const FontConfig &config)
    : library_(nullptr), face_(nullptr), config_(config) {
  if (FT_Init_FreeType(&library_)) {
    throw std::runtime_error("Falha ao iniciar FreeType.");
  }

  if (FT_New_Face(library_, config_.path.c_str(), 0, &face_)) {
    FT_Done_FreeType(library_);
    throw std::runtime_error("Falha ao carregar fonte: " + config_.path);
  }

  if (FT_Set_Pixel_Sizes(face_, 0, config_.size)) {
    FT_Done_Face(face_);
    FT_Done_FreeType(library_);
    throw std::runtime_error("Falha ao definir tamanho da fonte.");
  }
}

FontManager::~FontManager() {
  if (face_) {
    FT_Done_Face(face_);
  }
  if (library_) {
    FT_Done_FreeType(library_);
  }
}

int FontManager::getPixelSize() const {
  return config_.size;
}

const std::string &FontManager::getFontPath() const {
  return config_.path;
}

int FontManager::getLineHeight() const {
  return face_->size->metrics.height >> 6;
}

int FontManager::getAscender() const {
  return face_->size->metrics.ascender >> 6;
}

int FontManager::getDescender() const {
  return face_->size->metrics.descender >> 6;
}

/** Sem hinting só para ícones Nerd Font (PUA). Blocos e texto usam hinting forte para contornos nítidos. */
static bool isNerdFontIcon(uint32_t cp) {
  return (cp >= 0xE000u && cp <= 0xF8FFu);
}

/** Hinting mais forte = mais nítido (TARGET_NORMAL). TARGET_LIGHT deixa mais suave. */
static unsigned long getLoadFlags(uint32_t codepoint, bool render) {
  if (isNerdFontIcon(codepoint)) {
    return (render ? FT_LOAD_RENDER : FT_LOAD_DEFAULT) | FT_LOAD_NO_HINTING;
  }
  return (render ? FT_LOAD_RENDER : FT_LOAD_DEFAULT) | FT_LOAD_TARGET_NORMAL;
}

int FontManager::getGlyphAdvance(uint32_t codepoint) const {
  if (FT_Load_Char(face_, codepoint, getLoadFlags(codepoint, false))) {
    return getPixelSize();
  }
  return static_cast<int>(face_->glyph->advance.x >> 6);
}

GlyphBitmap FontManager::loadGlyph(uint32_t codepoint) const {
  {
    auto it = glyphCache_.find(codepoint);
    if (it != glyphCache_.end()) {
      return it->second;
    }
  }

  const unsigned long loadFlags = getLoadFlags(codepoint, true);

  if (FT_Load_Char(face_, codepoint, loadFlags)) {
    const uint32_t firstFallback = (codepoint >= 0xE000u && codepoint <= 0xF8FFu)
        ? 0xFFFDu
        : static_cast<uint32_t>('?');
    if (FT_Load_Char(face_, firstFallback, getLoadFlags(firstFallback, true)) &&
        FT_Load_Char(face_, static_cast<uint32_t>('?'), getLoadFlags(static_cast<uint32_t>('?'), true))) {
      return GlyphBitmap{};
    }
  }

  const FT_GlyphSlot glyph = face_->glyph;
  const FT_Bitmap& bitmap = glyph->bitmap;
  const int w = static_cast<int>(bitmap.width);
  const int h = static_cast<int>(bitmap.rows);

  GlyphBitmap result;
  result.width = w;
  result.height = h;
  result.bearingX = glyph->bitmap_left;
  result.bearingY = glyph->bitmap_top;
  result.advance = static_cast<int>(glyph->advance.x >> 6);
  result.pixels.assign(
      bitmap.buffer,
      bitmap.buffer + static_cast<std::size_t>(w * h));

  glyphCache_[codepoint] = result;
  return result;
}

/** colorR, colorG, colorB, colorA em 0–255 (sem conversão). */
void FontManager::drawGlyphBitmap(const GlyphBitmap &glyph, float x, float baselineY, bool bold, float colorR, float colorG, float colorB, float colorA) const {
  if (glyph.width <= 0 || glyph.height <= 0 || glyph.pixels.empty()) {
    return;
  }

  const int w = glyph.width;
  const int h = glyph.height;
  const float drawX = std::floor(x + static_cast<float>(glyph.bearingX) + 0.5f);
  const float drawY = std::floor(baselineY - static_cast<float>(glyph.bearingY) + 0.5f);

  const unsigned char R = static_cast<unsigned char>(std::min(255, std::max(0, static_cast<int>(colorR + 0.5f))));
  const unsigned char G = static_cast<unsigned char>(std::min(255, std::max(0, static_cast<int>(colorG + 0.5f))));
  const unsigned char B = static_cast<unsigned char>(std::min(255, std::max(0, static_cast<int>(colorB + 0.5f))));
  const int A255 = std::min(255, std::max(0, static_cast<int>(colorA + 0.5f)));

  std::vector<unsigned char> flipped(static_cast<size_t>(w * h));
  for (int row = 0; row < h; ++row) {
    const int srcRow = h - 1 - row;
    for (int col = 0; col < w; ++col) {
      flipped[static_cast<size_t>(row * w + col)] =
          glyph.pixels[static_cast<size_t>(srcRow * w + col)];
    }
  }

  std::vector<unsigned char> rgba(static_cast<size_t>(w * h * 4));
  for (int i = 0; i < w * h; ++i) {
    const int a = (static_cast<int>(flipped[static_cast<size_t>(i)]) * A255) / 255;
    rgba[static_cast<size_t>(i * 4 + 0)] = R;
    rgba[static_cast<size_t>(i * 4 + 1)] = G;
    rgba[static_cast<size_t>(i * 4 + 2)] = B;
    rgba[static_cast<size_t>(i * 4 + 3)] = static_cast<unsigned char>(std::min(255, std::max(0, a)));
  }

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glRasterPos2f(drawX, drawY + static_cast<float>(h));
  // [cor da fonte] GL_RGBA com buffer explícito; GL_ALPHA + cor atual não funcionava neste driver.
  glDrawPixels(w, h, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());

  if (bold) {
    glRasterPos2f(drawX + 1.0f, drawY + static_cast<float>(h));
    glDrawPixels(w, h, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
  }
}

void FontManager::drawText(const char *text, float x, float baselineY, bool bold, float colorR, float colorG, float colorB, float colorA) const {
  if (!text) {
    return;
  }

  float penX = x;
  for (const char *p = text; *p != '\0'; ++p) {
    const unsigned char ch = static_cast<unsigned char>(*p);
    const GlyphBitmap glyph = loadGlyph(static_cast<uint32_t>(ch));
    drawGlyphBitmap(glyph, penX, baselineY, bold, colorR, colorG, colorB, colorA);
    penX += static_cast<float>(glyph.advance);
  }
}

void FontManager::drawText(const char32_t *text, float x, float baselineY, bool bold, float colorR, float colorG, float colorB, float colorA) const {
  if (!text) {
    return;
  }

  float penX = x;
  for (const char32_t *p = text; *p != U'\0'; ++p) {
    uint32_t codepoint = static_cast<uint32_t>(*p);

    if (codepoint == 0) {
      break;
    }

    GlyphBitmap glyph = loadGlyph(codepoint);
    drawGlyphBitmap(glyph, penX, baselineY, bold, colorR, colorG, colorB, colorA);
    penX += static_cast<float>(glyph.advance);
  }
}