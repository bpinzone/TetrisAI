#ifndef BOARD_DERIV_H
#define BOARD_DERIV_H

#include "board_foundation.h"

// historic comment
// === Secondary Cache. Relies on info in Primary Cache being up to date to compute these.
/*
// Cached second. Should be updated in update_secondary_cache().
Update cache is responsible for the following.
None of these are ever read by place_block() or clear_row()
*/
struct Board_deriv {

    const inline static int c_max_tetris_mode_height = 6;
    const inline static int c_height_diff_punishment_thresh = 3;

    Board_deriv();
    void reset();
    void update(const Board_foundation& foundation);

    int num_trenches;
    bool at_least_one_side_clear;
    int lowest_height;
    int second_lowest_height;
    int highest_height;
    int sum_of_squared_heights;

    // Assuming no holes, is true iff a cyan could be placed for a tetris right now.
    bool is_tetrisable;

    bool is_clear;
    bool has_good_trench_status;
    int num_holes;

    bool in_tetris_mode;
    bool receives_height_punishment;
};

#endif