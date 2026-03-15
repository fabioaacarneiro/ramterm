#pragma once

#include "config/Config.hpp"
#include "font/FontManager.hpp"
#include "pty/PtyProcess.hpp"
#include "renderer/Renderer.hpp"
#include "terminal/VTermEngine.hpp"
#include "window/Window.hpp"

class Application {
public:
  Application();
  void run();
  void onResize(int pixelWidth, int pixelHeight);
  /** Retorna true se leu algum dado do PTY (conteúdo do terminal mudou). */
  bool pumpPty();

private:
  void pixelToCell(double x, double y, int& row, int& col) const;
  /** Se o fim da seleção caiu na linha seguinte com col 0 (arraste “só uma linha”), mantém na mesma linha usando coluna do x. */
  void pixelToCellFromCursor(double x, double y, int& row, int& col) const;
  int columnFromX(double x, int cols, int pixelWidth) const;
  void snapSelectionEndToSameLineIfAccidental(double x, int& endRow, int& endCol, int cols, int pixelWidth) const;
  void getWordBounds(int row, int col, int& startCol, int& endCol) const;
  /** Atualiza métricas do renderer a partir do fontManager e redimensiona o grid/PTY. */
  void applyFontMetricsAndResize();

  AppConfig config_;
  Window window_;
  FontManager fontManager_;
  Renderer renderer_;
  PtyProcess pty_;
  VTermEngine terminal_;
  int windowWidth_ = 1280;
  int windowHeight_ = 720;
  bool selecting_ = false;
  bool selHasMoved_ = false;
  int selAnchorRow_ = 0;
  int selAnchorCol_ = 0;
  double lastPressTime_ = 0.0;
  int lastPressRow_ = -1;
  int lastPressCol_ = -1;
  int lastClickCount_ = 1;
  int lastDragRow_ = 0;
  int lastDragCol_ = 0;
  mutable bool requestRender_ = false;
};