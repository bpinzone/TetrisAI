#ifndef BOARD_FOUNDATION_H
#define BOARD_FOUNDATION_H

#include <array>

#include "grid.h"
#include "block.h"
#include "board_size.h"

struct Board_foundation {

    Board_foundation(bool has_colors);

    void reset();

    // truly fundamental.
    Grid grid;
    const Block* current_hold;
    bool just_swapped;

    // Really the first derivative, but these are so tightly coupled, we'll keep them with the foundation.
    // === Primary Cache. Should be updated in place_block() and clear_row(), and add_junk() ===
    std::array<int, BoardSize::c_cols> height_map;
    int num_cells_filled;
    // If there are 0 holes, num_cells_filled will be equal to this.
    int perfect_num_cells_filled;
};


#endif