#ifndef BIT_GRID_H
#define BIT_GRID_H

#include <cstdint>
#include <bitset>

#include "board_size.h"


class BitGrid {
public:
    void set_at(size_t row, size_t col, bool filled);
    void clear_row(size_t deleted_row);

    // returns true iff the game is over.
    bool add_junk(int pos, int count);

    size_t count() const;
    bool get_at(size_t row, size_t col) const;

    // returns true iff every cell in all the rows in the range is equal to value.
    bool is_row_range_all_equal_to(size_t row_begin, size_t row_end, bool value) const;

    bool is_column_clear(size_t col) const;

private:

    using InternalGrid_t = std::bitset<BoardSize::c_size>;

    // (0, 0) is bottom left;  (1, 0) is 2nd row, 1st column;  (0, 1) is 1st row, 2nd column.
    InternalGrid_t::reference ref_at(size_t row, size_t col);
    bool const_at(size_t row, size_t col) const;

    InternalGrid_t board;


    /*
    row=0, col=0 is at index c_size - 1.

    Below is laid out visually like the board, and the text is the bit index.                                                least sig vv
    (c_size - 37), (c_size - 38), (c_size - 39), (c_size - 40), (c_size - 41), (c_size - 42), (c_size - 43), (c_size - 44), (c_size - 45)
    (c_size - 28), (c_size - 29), (c_size - 30), (c_size - 31), (c_size - 32), (c_size - 33), (c_size - 34), (c_size - 35), (c_size - 36)
    (c_size - 19), (c_size - 20), (c_size - 21), (c_size - 22), (c_size - 23), (c_size - 24), (c_size - 25), (c_size - 26), (c_size - 27)
    (c_size - 10), (c_size - 11), (c_size - 12), (c_size - 13), (c_size - 14), (c_size - 15), (c_size - 16), (c_size - 17), (c_size - 18)
    (c_size - 1),  (c_size - 2),  (c_size - 3),  (c_size - 4),  (c_size - 5),  (c_size - 6),  (c_size - 7),  (c_size - 8),  (c_size - 9)
    most sig^

    left shifts will move things in the board left or down.
    right shifts will move things in the board right or up.
    */
    static size_t flat_idx(size_t row, size_t col);
    static InternalGrid_t shift_board_down(const InternalGrid_t& board, size_t num_rows);
    static InternalGrid_t shift_board_up(const InternalGrid_t& board, size_t num_rows);

};




#endif