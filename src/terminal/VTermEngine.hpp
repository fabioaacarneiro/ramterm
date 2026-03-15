#pragma once

#include "config/Config.hpp"
#include "terminal/VTermScreenBuffer.hpp"
#include <deque>
#include <string>
#include <vector>
#include <vterm.h>

class VTermEngine {
public:
  VTermEngine(int rows, int columns);
  ~VTermEngine();

  /** Aplica tema (default fg/bg e paleta 0–15). Deve ser chamado após construção. */
  void setTheme(const ThemeConfig& theme);

  VTermEngine(const VTermEngine&) = delete;
  VTermEngine& operator=(const VTermEngine&) = delete;

  void resize(int rows, int columns);
  void feed(const std::string& bytes);
  void reset(bool hard);

  int getRows() const;
  int getColumns() const;

  const VTermScreenBuffer& getPrimaryBuffer() const;
  const VTermScreenBuffer& getActiveBuffer() const;
  bool isUsingAltScreen() const;

  /** Desloca a view do scrollback; delta > 0 = rolar para cima (ver mais acima). */
  void addScrollOffset(int delta);
  int getScrollOffset() const { return scrollOffset_; }
  int getScrollbackLines() const { return static_cast<int>(scrollback_.size()); }

  /** Seleção em coordenadas da tela visível (0-based). -1 = sem seleção. */
  void setSelection(int startRow, int startCol, int endRow, int endCol);
  void getSelection(int& startRow, int& startCol, int& endRow, int& endCol) const;
  bool hasSelection() const;
  /** Texto da seleção atual em UTF-8 (usa o buffer visível). */
  std::string getSelectedText() const;

private:
  void buildViewBuffer() const;
  static int damageCallback(VTermRect rect, void* user);
  static int moverectCallback(VTermRect dest, VTermRect src, void* user);
  static int movecursorCallback(VTermPos pos, VTermPos oldpos, int visible, void* user);
  static int settermpropCallback(VTermProp prop, VTermValue* value, void* user);
  static int bellCallback(void* user);
  static int sbPushlineCallback(int cols, const VTermScreenCell* cells, void* user);
  static int sbPoplineCallback(int cols, VTermScreenCell* cells, void* user);

  void syncRect(const VTermRect& rect);
  void syncAll();
  void syncCursor();
  void writeCellToBuffer(VTermScreenBuffer& buffer, int row, int column);

  TermCell convertCell(const VTermScreenCell& source) const;
  TermColor colorFromVTerm(VTermColor color) const;

private:
  int rows_;
  int columns_;
  VTerm* vterm_;
  VTermState* state_;
  VTermScreen* screen_;
  bool usingAltScreen_;
  bool cursorVisible_;

  static constexpr size_t kMaxScrollbackLines = 10000u;

  VTermScreenBuffer primaryBuffer_;
  VTermScreenBuffer altBuffer_;
  mutable VTermScreenBuffer viewBuffer_;
  std::deque<std::vector<TermCell>> scrollback_;
  int scrollOffset_ = 0;

  int selStartRow_ = -1;
  int selStartCol_ = -1;
  int selEndRow_ = -1;
  int selEndCol_ = -1;
};