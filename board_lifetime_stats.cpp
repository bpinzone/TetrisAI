#include "board_lifetime_stats.h"

#include <iostream>

Board_lifetime_stats::Board_lifetime_stats(){
    reset();
}

void Board_lifetime_stats::reset(){
    num_blocks_placed = 0;
    num_placements_that_cleared_rows = 0;
    num_tetrises = 0;
    num_non_tetrises = 0;
    num_all_clears = 0;
    max_height_exp_moving_average = 0;
}

void Board_lifetime_stats::update(
    const Board_foundation& foundation,
    const Board_deriv& deriv,
    int num_rows_cleared_just_now)
{
    ++num_blocks_placed;
    if(num_rows_cleared_just_now > 0){
        ++num_placements_that_cleared_rows;
        if(num_rows_cleared_just_now == 4){
            ++num_tetrises;
        }
        else{
            ++num_non_tetrises;
        }
    }
    if(deriv.is_clear){
        ++num_all_clears;
    }
    max_height_exp_moving_average =
        (0.5 * max_height_exp_moving_average) +
        (0.5 * deriv.highest_height);

}

std::ostream& operator<<(std::ostream& os, const Board_lifetime_stats& stats){
    os << "All clears: " << stats.num_all_clears << "\n";
    os << "Tetrises: " << stats.num_tetrises << "\n";
    return os;

}