#ifndef BIT_GRID_H
#define BIT_GRID_H

#include <cstdint>
#include <bitset>

#include "board_size.h"


class BitGrid {
public:
    void set_at(size_t row, size_t col, bool filled);
    void clear_row(size_t deleted_row);

    size_t count() const;
    bool get_at(size_t row, size_t col) const;

private:

    using InternalGrid_t = std::bitset<BoardSize::c_size>;

    // (0, 0) is bottom left;  (1, 0) is 2nd row, 1st column;  (0, 1) is 1st row, 2nd column.
    InternalGrid_t::reference ref_at(size_t row, size_t col);
    bool const_at(size_t row, size_t col) const;

    InternalGrid_t board;

};




#endif