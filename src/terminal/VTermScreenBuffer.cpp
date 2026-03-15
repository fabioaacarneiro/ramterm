#include "terminal/VTermScreenBuffer.hpp"

VTermScreenBuffer::VTermScreenBuffer() = default;

VTermScreenBuffer::VTermScreenBuffer(int rows, int columns) {
  resize(rows, columns);
}

void VTermScreenBuffer::resize(int rows, int columns) {
  rows = std::max(1, rows);
  columns = std::max(1, columns);

  std::vector<TermCell> newCells(static_cast<size_t>(rows * columns));

  const int copyRows = std::min(rows_, rows);
  const int copyCols = std::min(columns_, columns);

  for (int r = 0; r < copyRows; ++r) {
    for (int c = 0; c < copyCols; ++c) {
      newCells[static_cast<size_t>(r * columns + c)] =
          cells_[static_cast<size_t>(r * columns_ + c)];
    }
  }

  rows_ = rows;
  columns_ = columns;
  cells_ = std::move(newCells);

  cursor_.row = std::clamp(cursor_.row, 0, rows_ - 1);
  cursor_.column = std::clamp(cursor_.column, 0, columns_ - 1);
}

void VTermScreenBuffer::clear() {
  std::fill(cells_.begin(), cells_.end(), TermCell{});
}

int VTermScreenBuffer::rows() const {
  return rows_;
}

int VTermScreenBuffer::columns() const {
  return columns_;
}

const TermCell& VTermScreenBuffer::cell(int row, int column) const {
  static const TermCell empty{};
  if (!valid(row, column)) {
    return empty;
  }
  return cells_[static_cast<size_t>(index(row, column))];
}

TermCell& VTermScreenBuffer::cell(int row, int column) {
  static TermCell dummy{};
  if (!valid(row, column)) {
    return dummy;
  }
  return cells_[static_cast<size_t>(index(row, column))];
}

void VTermScreenBuffer::setCell(int row, int column, const TermCell& value) {
  if (!valid(row, column)) {
    return;
  }
  cells_[static_cast<size_t>(index(row, column))] = value;
}

void VTermScreenBuffer::setCursor(const TermCursor& cursor) {
  cursor_ = cursor;
  cursor_.row = std::clamp(cursor_.row, 0, std::max(0, rows_ - 1));
  cursor_.column = std::clamp(cursor_.column, 0, std::max(0, columns_ - 1));
}

const TermCursor& VTermScreenBuffer::cursor() const {
  return cursor_;
}

int VTermScreenBuffer::index(int row, int column) const {
  return row * columns_ + column;
}

bool VTermScreenBuffer::valid(int row, int column) const {
  return row >= 0 && row < rows_ && column >= 0 && column < columns_;
}