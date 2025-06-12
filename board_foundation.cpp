#include "board_foundation.h"

Board_foundation::Board_foundation(bool has_colors) : grid(has_colors) {
    reset();
}

void Board_foundation::reset() {
    current_hold = nullptr;
    just_swapped = false;

    height_map = std::array<int, BoardSize::c_cols>{0};
    num_cells_filled = 0;
    perfect_num_cells_filled = 0;

}