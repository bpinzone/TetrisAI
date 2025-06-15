#ifndef BOARD_FOUNDATION_H
#define BOARD_FOUNDATION_H

#include <array>

#include "grid.h"
#include "block.h"
#include "board_size.h"
#include <iosfwd>

// todo: make as much private as possible.
struct Board_foundation {

    // modifying
    Board_foundation(bool has_colors);
    Board_foundation(bool has_colors, std::istream& is);

    friend std::ostream& operator<<(std::ostream& os, const Board_foundation& foundation);

    void reset();
    void clear_row(int deleted_row);
    // returns true iff board is still promising (is game over)
    bool place_block_no_clearing(const Block& b, Placement p, int *min_row_x_affected, int *max_row_x_affected);

    // constant
    bool is_row_full(int row) const;

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

private:
    int get_height_map_reduction(int deleted_row, int query_col) const;
    int get_row_after_drop(const Block& b, Placement p) const;
    int compute_height(size_t col_x) const;

};


#endif