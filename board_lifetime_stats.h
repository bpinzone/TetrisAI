#ifndef BOARD_LIFETIME_STATS_H
#define BOARD_LIFETIME_STATS_H

#include <iosfwd>

#include "board_foundation.h"
#include "board_deriv.h"

struct Board_lifetime_stats {

    Board_lifetime_stats();
    void reset();
    void update(
        const Board_foundation& foundation,
        const Board_deriv& deriv,
        int num_rows_cleared_just_now);

    int num_blocks_placed;
    int num_placements_that_cleared_rows;
    int num_tetrises;
    int num_non_tetrises;
    int num_all_clears;
    double max_height_exp_moving_average;

    friend std::ostream& operator<<(std::ostream& os, const Board_lifetime_stats& stats);
};


#endif