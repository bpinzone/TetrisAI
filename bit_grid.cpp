#include "bit_grid.h"
#include <iostream>

void BitGrid::set_at(size_t row, size_t col, bool filled){
    ref_at(row, col) = filled;
}

void BitGrid::clear_row(size_t deleted_row) {

    InternalGrid_t board_shifted_down = board << BoardSize::c_cols;

    InternalGrid_t below_del_row_mask;
    below_del_row_mask.set();
    below_del_row_mask <<= (BoardSize::c_cols * (BoardSize::c_rows - deleted_row));

    InternalGrid_t above_including_del_row_mask{~below_del_row_mask};

    board =
        (board & below_del_row_mask) |
        (board_shifted_down & above_including_del_row_mask);

}

size_t BitGrid::count() const{
    return board.count();
}

bool BitGrid::get_at(size_t row, size_t col) const{
    return const_at(row, col);
}

BitGrid::InternalGrid_t::reference
BitGrid::ref_at(size_t row, size_t col){

    size_t idx = BoardSize::c_size - 1 - ((row * BoardSize::c_cols) + col);
    return board[idx];
}

bool BitGrid::const_at(size_t row, size_t col) const {
    size_t idx = BoardSize::c_size - 1 - ((row * BoardSize::c_cols) + col);
    return board[idx];
}

