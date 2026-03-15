#include "terminal/VTermEngine.hpp"
#include <algorithm>
#include <stdexcept>

VTermEngine::VTermEngine(int rows, int columns)
    : rows_(std::max(1, rows)),
      columns_(std::max(1, columns)),
      vterm_(nullptr),
      state_(nullptr),
      screen_(nullptr),
  usingAltScreen_(false),
  cursorVisible_(true),
  primaryBuffer_(rows_, columns_),
  altBuffer_(rows_, columns_),
  viewBuffer_(rows_, columns_),
  scrollOffset_(0) {
  vterm_ = vterm_new(rows_, columns_);
  if (!vterm_) {
    throw std::runtime_error("Failed to create VTerm");
  }

  vterm_set_utf8(vterm_, 1);

  state_ = vterm_obtain_state(vterm_);
  screen_ = vterm_obtain_screen(vterm_);

  static VTermScreenCallbacks callbacks{};
  callbacks.damage = &VTermEngine::damageCallback;
  callbacks.moverect = &VTermEngine::moverectCallback;
  callbacks.movecursor = &VTermEngine::movecursorCallback;
  callbacks.settermprop = &VTermEngine::settermpropCallback;
  callbacks.bell = &VTermEngine::bellCallback;
  callbacks.sb_pushline = &VTermEngine::sbPushlineCallback;
  callbacks.sb_popline = &VTermEngine::sbPoplineCallback;

  vterm_screen_set_callbacks(screen_, &callbacks, this);
  vterm_screen_enable_altscreen(screen_, 1);
  vterm_screen_reset(screen_, 1);

  syncAll();
}

void VTermEngine::setTheme(const ThemeConfig& theme) {
  VTermColor defaultFg, defaultBg;
  vterm_color_rgb(&defaultFg,
      static_cast<uint8_t>(std::min(255, std::max(0, static_cast<int>(theme.font.r)))),
      static_cast<uint8_t>(std::min(255, std::max(0, static_cast<int>(theme.font.g)))),
      static_cast<uint8_t>(std::min(255, std::max(0, static_cast<int>(theme.font.b)))));
  vterm_color_rgb(&defaultBg,
      static_cast<uint8_t>(std::min(255, std::max(0, static_cast<int>(theme.background.r)))),
      static_cast<uint8_t>(std::min(255, std::max(0, static_cast<int>(theme.background.g)))),
      static_cast<uint8_t>(std::min(255, std::max(0, static_cast<int>(theme.background.b)))));
  vterm_state_set_default_colors(state_, &defaultFg, &defaultBg);
  vterm_screen_set_default_colors(screen_, &defaultFg, &defaultBg);

  for (int i = 0; i < 16; ++i) {
    VTermColor c;
    vterm_color_rgb(&c,
        static_cast<uint8_t>(std::min(255, std::max(0, static_cast<int>(theme.palette[i].r)))),
        static_cast<uint8_t>(std::min(255, std::max(0, static_cast<int>(theme.palette[i].g)))),
        static_cast<uint8_t>(std::min(255, std::max(0, static_cast<int>(theme.palette[i].b)))));
    vterm_state_set_palette_color(state_, i, &c);
  }
}

VTermEngine::~VTermEngine() {
  if (vterm_) {
    vterm_free(vterm_);
    vterm_ = nullptr;
  }
}

void VTermEngine::resize(int rows, int columns) {
  rows_ = std::max(1, rows);
  columns_ = std::max(1, columns);

  primaryBuffer_.resize(rows_, columns_);
  altBuffer_.resize(rows_, columns_);
  viewBuffer_.resize(rows_, columns_);
  scrollback_.clear();
  scrollOffset_ = 0;

  vterm_set_size(vterm_, rows_, columns_);
  vterm_screen_flush_damage(screen_);
  syncAll();
}

void VTermEngine::feed(const std::string& bytes) {
  if (bytes.empty()) {
    return;
  }

  vterm_input_write(vterm_, bytes.data(), bytes.size());
  vterm_screen_flush_damage(screen_);
}

void VTermEngine::reset(bool hard) {
  vterm_screen_reset(screen_, hard ? 1 : 0);
  if (hard) {
    scrollback_.clear();
    scrollOffset_ = 0;
  }
  syncAll();
}

int VTermEngine::getRows() const {
  return rows_;
}

int VTermEngine::getColumns() const {
  return columns_;
}

const VTermScreenBuffer& VTermEngine::getPrimaryBuffer() const {
  return primaryBuffer_;
}

const VTermScreenBuffer& VTermEngine::getActiveBuffer() const {
  if (usingAltScreen_) return altBuffer_;
  if (scrollOffset_ == 0) return primaryBuffer_;
  buildViewBuffer();
  return viewBuffer_;
}

void VTermEngine::addScrollOffset(int delta) {
  scrollOffset_ = std::max(0, std::min(scrollOffset_ + delta, static_cast<int>(scrollback_.size())));
}

void VTermEngine::buildViewBuffer() const {
  const int sb = static_cast<int>(scrollback_.size());
  if (scrollOffset_ <= 0 || scrollOffset_ > sb || scrollOffset_ > rows_) return;
  for (int r = 0; r < scrollOffset_; ++r) {
    const size_t sbIdx = scrollback_.size() - static_cast<size_t>(scrollOffset_) + static_cast<size_t>(r);
    const std::vector<TermCell>& line = scrollback_[sbIdx];
    for (int c = 0; c < columns_; ++c) {
      viewBuffer_.setCell(r, c, line[static_cast<size_t>(c)]);
    }
  }
  for (int r = scrollOffset_; r < rows_; ++r) {
    for (int c = 0; c < columns_; ++c) {
      viewBuffer_.setCell(r, c, primaryBuffer_.cell(r - scrollOffset_, c));
    }
  }
  TermCursor cur = primaryBuffer_.cursor();
  cur.row = cur.row + scrollOffset_;
  viewBuffer_.setCursor(cur);
}

bool VTermEngine::isUsingAltScreen() const {
  return usingAltScreen_;
}

int VTermEngine::damageCallback(VTermRect rect, void* user) {
  auto* self = static_cast<VTermEngine*>(user);
  self->syncRect(rect);
  self->syncCursor();
  return 1;
}

int VTermEngine::moverectCallback(VTermRect, VTermRect, void* user) {
  auto* self = static_cast<VTermEngine*>(user);
  self->syncAll();
  return 1;
}

int VTermEngine::movecursorCallback(VTermPos, VTermPos, int visible, void* user) {
  auto* self = static_cast<VTermEngine*>(user);
  self->cursorVisible_ = (visible != 0);
  self->syncCursor();
  return 1;
}

int VTermEngine::settermpropCallback(VTermProp prop, VTermValue* value, void* user) {
  auto* self = static_cast<VTermEngine*>(user);

  if (prop == VTERM_PROP_ALTSCREEN) {
    self->usingAltScreen_ = value->boolean != 0;
    self->syncAll();
    self->syncCursor();
  } else if (prop == VTERM_PROP_CURSORVISIBLE) {
    self->cursorVisible_ = value->boolean != 0;
    self->syncCursor();
  }

  return 1;
}

int VTermEngine::bellCallback(void*) {
  return 1;
}

int VTermEngine::sbPushlineCallback(int cols, const VTermScreenCell* cells, void* user) {
  auto* self = static_cast<VTermEngine*>(user);
  if (!self || cols <= 0 || !cells) return 1;
  const int colsClamp = std::min(cols, self->columns_);
  std::vector<TermCell> line(static_cast<size_t>(self->columns_));
  for (int c = 0; c < colsClamp; ++c) {
    line[static_cast<size_t>(c)] = self->convertCell(cells[c]);
  }
  self->scrollback_.push_back(std::move(line));
  while (self->scrollback_.size() > VTermEngine::kMaxScrollbackLines) {
    self->scrollback_.pop_front();
  }
  return 1;
}

int VTermEngine::sbPoplineCallback(int, VTermScreenCell*, void*) {
  return 0;
}

void VTermEngine::syncRect(const VTermRect& rect) {
  VTermScreenBuffer& buffer = usingAltScreen_ ? altBuffer_ : primaryBuffer_;

  const int startRow = std::max(0, rect.start_row);
  const int endRow = std::min(rows_, rect.end_row);
  const int startCol = std::max(0, rect.start_col);
  const int endCol = std::min(columns_, rect.end_col);

  for (int row = startRow; row < endRow; ++row) {
    for (int column = startCol; column < endCol; ++column) {
      writeCellToBuffer(buffer, row, column);
    }
  }
}

void VTermEngine::syncAll() {
  VTermRect rect{};
  rect.start_row = 0;
  rect.end_row = rows_;
  rect.start_col = 0;
  rect.end_col = columns_;

  primaryBuffer_.clear();
  altBuffer_.clear();

  syncRect(rect);
  syncCursor();
}

void VTermEngine::syncCursor() {
  VTermPos pos{};
  vterm_state_get_cursorpos(state_, &pos);

  TermCursor cursor;
  cursor.row = pos.row;
  cursor.column = pos.col;
  cursor.visible = cursorVisible_;

  if (usingAltScreen_) {
    altBuffer_.setCursor(cursor);
  } else {
    primaryBuffer_.setCursor(cursor);
  }
}

void VTermEngine::writeCellToBuffer(VTermScreenBuffer& buffer, int row, int column) {
  VTermPos pos{};
  pos.row = row;
  pos.col = column;

  VTermScreenCell screenCell{};
  if (vterm_screen_get_cell(screen_, pos, &screenCell) == 0) {
    return;
  }

  buffer.setCell(row, column, convertCell(screenCell));
}

TermCell VTermEngine::convertCell(const VTermScreenCell& source) const {
  TermCell cell;

  cell.nchars = 0;
  for (int i = 0; i < VTERM_MAX_CHARS_PER_CELL && source.chars[i] != 0; ++i) {
    cell.codepoints[i] = static_cast<uint32_t>(source.chars[i]);
    cell.nchars = i + 1;
  }
  if (cell.nchars == 0) {
    cell.codepoint = static_cast<uint32_t>(' ');
    cell.codepoints[0] = static_cast<uint32_t>(' ');
  } else {
    cell.codepoint = cell.codepoints[0];
  }

  cell.width = (source.width >= 2) ? 2 : 1;
  const uint32_t first = cell.codepoint;
  if (first >= 0x2580u && first <= 0x259Fu) {
    cell.width = 1;
  }

  cell.foreground = colorFromVTerm(source.fg);
  cell.background = colorFromVTerm(source.bg);

  cell.bold = source.attrs.bold != 0;
  cell.underline = source.attrs.underline != 0;
  cell.inverse = source.attrs.reverse != 0;

  if (cell.inverse) {
    const TermColor temp = cell.foreground;
    cell.foreground = cell.background;
    cell.background = temp;
  }

  return cell;
}

TermColor VTermEngine::colorFromVTerm(VTermColor color) const {
  vterm_screen_convert_color_to_rgb(screen_, &color);

  TermColor result;
  result.r = static_cast<float>(color.rgb.red) / 255.0f;
  result.g = static_cast<float>(color.rgb.green) / 255.0f;
  result.b = static_cast<float>(color.rgb.blue) / 255.0f;
  result.a = 1.0f;
  return result;
}