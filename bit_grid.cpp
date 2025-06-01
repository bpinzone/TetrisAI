#include "bit_grid.h"
#include "board_size.h"
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


bool BitGrid::add_junk(int pos, int count){
    if(count <= 0){
        throw std::logic_error("count must be positive");
    }
    const bool will_survive = is_row_range_all_equal_to(BoardSize::c_rows - count, BoardSize::c_rows, false);

    auto shifted_up = BitGrid::shift_board_up(board, count);

    for(int row = 0; row < count; ++row){
        for(int col = 0; col < BoardSize::c_cols; ++col){
            const bool fill_it = col != pos;
            shifted_up[flat_idx(row, col)] = fill_it;
        }
    }
    board = shifted_up;
    return !will_survive;
}

size_t BitGrid::count() const{
    return board.count();
}

bool BitGrid::get_at(size_t row, size_t col) const{
    return const_at(row, col);
}

bool BitGrid::is_row_range_all_equal_to(size_t row_begin, size_t row_end, bool value) const {

    const InternalGrid_t interested_starts_at_bottom = BitGrid::shift_board_down(board, row_begin);
    const size_t num_interested_rows = row_end - row_begin;
    const size_t num_uninterested_rows = BoardSize::c_rows - num_interested_rows;
    const InternalGrid_t interested_ends_at_top = BitGrid::shift_board_up(interested_starts_at_bottom, num_uninterested_rows);
    const InternalGrid_t &interested_only = interested_ends_at_top;

    if(value){
        return interested_only.count() == num_interested_rows * BoardSize::c_cols;
    }
    else{
        return interested_only.count() == 0;
    }
}

bool BitGrid::is_column_clear(size_t col) const {
    for(size_t row = 0; row < BoardSize::c_rows; ++row){
        if(get_at(row, col)){
            return false;
        }
    }
    return true;
}

BitGrid::InternalGrid_t::reference
BitGrid::ref_at(size_t row, size_t col){

    return board[BitGrid::flat_idx(row, col)];
}

// (0, 0) is bottom left;  (1, 0) is 2nd row, 1st column;  (0, 1) is 1st row, 2nd column.
bool BitGrid::const_at(size_t row, size_t col) const {

    return board[BitGrid::flat_idx(row, col)];
}

size_t BitGrid::flat_idx(size_t row, size_t col){
    return BoardSize::c_size - 1 - ((row * BoardSize::c_cols) + col);
}

BitGrid::InternalGrid_t BitGrid::shift_board_down(const InternalGrid_t& board, size_t num_rows){
    return board << (num_rows * BoardSize::c_cols);
}
BitGrid::InternalGrid_t BitGrid::shift_board_up(const InternalGrid_t& board, size_t num_rows){
    return board >> (num_rows * BoardSize::c_cols);
}