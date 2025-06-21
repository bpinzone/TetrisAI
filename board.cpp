#include "board.h"
#include "board_size.h"

#include "block.h"
#include "utility.h"
#include "global_stats.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <utility>
#include <numeric>
#include <iostream>

// TODO: Replace with individual using statements
using namespace std;

Board::Board(istream& is) :foundation(false, is) {

    update_secondary_cache();
    load_ancestral_data_with_current_data();
}

ostream& operator<<(ostream& os, const Board& s) {

    os << s.foundation;

    os << "All clears: " << s.lifetime_stats.num_all_clears << "\n";
    os << "Tetrises: " << s.lifetime_stats.num_tetrises << "\n";
    os << "------------------------------------------------\n";

    return os;
}

// Return true iff this board is still promising.
// todo: revamp what this function means. clearly distinguish between game over and not promising.
// todo: revamp what this function means. clearly distinguish between game over and not promising.
// todo: revamp what this function means. clearly distinguish between game over and not promising.
bool Board::place_block(const Block& b, Placement p){

    assert(!p.get_is_hold());

    int min_row_x_affected;
    int max_row_x_affected;
    const bool game_over = foundation.place_block_no_clearing(b, p,
        &min_row_x_affected, &max_row_x_affected);
    if(game_over){
        return false;
    }

    // Check for cleared rows
    const int num_rows_cleared_just_now = foundation.check_and_clear_rows(
        min_row_x_affected, max_row_x_affected);

    // must be called before is_promising.
    update_secondary_cache();

    // TODO: remove this? questionable.... this was a pruning hack probably.
    // TODO: removing this for now. Might play a different game...
    // if(!is_promising()){
    //     return false;
    // }

    update_lifetime_cache(num_rows_cleared_just_now);
    foundation.just_swapped = false;
    return true;
}

const Block* Board::swap_block(const Block& b){
    const Block* old_hold = foundation.current_hold;
    foundation.current_hold = &b;
    foundation.just_swapped = true;
    return old_hold;
}

void Board::set_lifetime_stats(const Board_lifetime_stats& new_lifetime_stats){
    lifetime_stats = new_lifetime_stats;
}

bool Board::has_greater_utility_than(const Board& other) const {

    ++gs_num_comparisons;

    // if(lifetime_stats.num_all_clears != other.lifetime_stats.num_all_clears){
    //     return lifetime_stats.num_all_clears > other.lifetime_stats.num_all_clears;
    // }

    // You are in tetris mode if you are here or less in height.
    static const int c_max_tetris_mode_height = 6;
    static const int c_height_diff_punishment_thresh = 3;

    const bool this_in_tetris_mode = deriv.highest_height <= c_max_tetris_mode_height;
    const bool other_in_tetris_mode = other.deriv.highest_height <= c_max_tetris_mode_height;

    // === Fundamental Priorities ===
    if(deriv.has_good_trench_status != other.deriv.has_good_trench_status){
        return deriv.has_good_trench_status;
    }
    // Holes
    const int this_holes = get_num_holes();
    const int other_holes = other.get_num_holes();
    if(this_holes != other_holes){
        return this_holes < other_holes;
    }

    // Prefer to be in Tetris mode.
    if(this_in_tetris_mode != other_in_tetris_mode){
        return this_in_tetris_mode;
    }

    if(this_in_tetris_mode && other_in_tetris_mode){
        if(deriv.at_least_one_side_clear != other.deriv.at_least_one_side_clear){
            return deriv.at_least_one_side_clear;
        }
        if(lifetime_stats.num_non_tetrises != other.lifetime_stats.num_non_tetrises){
            return lifetime_stats.num_non_tetrises < other.lifetime_stats.num_non_tetrises;
        }
    }

    // Keep relatively even except for the one trench.
    const bool this_receives_height_punishment = deriv.highest_height - deriv.second_lowest_height >= c_height_diff_punishment_thresh;
    const bool other_receives_height_punishment = other.deriv.highest_height - other.deriv.second_lowest_height >= c_height_diff_punishment_thresh;
    if(this_receives_height_punishment != other_receives_height_punishment){
        return !this_receives_height_punishment;
    }

    if(this_in_tetris_mode){

        // Get the tetrises
        if(lifetime_stats.num_tetrises != other.lifetime_stats.num_tetrises){
            return lifetime_stats.num_tetrises > other.lifetime_stats.num_tetrises;
        }

        // Become tetrisable
        if(deriv.is_tetrisable != other.deriv.is_tetrisable){
            return deriv.is_tetrisable;
        }

        // Build up
        // Make the 2nd shortest column as large as possible.
        // Encourges alg to build a solid mass of blocks, but not clear rows,
        // in order to get to the point where we can forsee being tetris-able.
        if(deriv.second_lowest_height != other.deriv.second_lowest_height){
            return deriv.second_lowest_height > other.deriv.second_lowest_height;
        }

        if(deriv.sum_of_squared_heights != other.deriv.sum_of_squared_heights){
            return deriv.sum_of_squared_heights < other.deriv.sum_of_squared_heights;
        }
        return lifetime_stats.max_height_exp_moving_average < other.lifetime_stats.max_height_exp_moving_average;

    }
    else{

        if(deriv.num_trenches != other.deriv.num_trenches){
            return deriv.num_trenches < other.deriv.num_trenches;
        }

        if(foundation.num_cells_filled != other.foundation.num_cells_filled){
            return foundation.num_cells_filled < other.foundation.num_cells_filled;
        }

        if(deriv.sum_of_squared_heights != other.deriv.sum_of_squared_heights){
            return deriv.sum_of_squared_heights < other.deriv.sum_of_squared_heights;
        }
        return lifetime_stats.max_height_exp_moving_average < other.lifetime_stats.max_height_exp_moving_average;
    }

}

int Board::get_num_holes() const {
    return foundation.perfect_num_cells_filled - foundation.num_cells_filled;
}

bool Board::can_swap_block(const Block& b) const {
    if(&b == foundation.current_hold){
        return false;
    }
    return !foundation.just_swapped;
}

bool Board::is_holding_some_block() const {
    return foundation.current_hold;
}

const Block * Board::get_hold() const {
    if(foundation.current_hold){
        return foundation.current_hold;
    }
    return nullptr;
}

int Board::get_num_blocks_placed() const {
    return lifetime_stats.num_blocks_placed;
}

double Board::get_tetris_percent() const {
    return static_cast<double>(lifetime_stats.num_tetrises) / lifetime_stats.num_blocks_placed * 100;
}

bool Board::has_more_cleared_rows_than(const Board& other) const {
    return lifetime_stats.num_placements_that_cleared_rows > other.lifetime_stats.num_placements_that_cleared_rows;
}

bool Board::is_clear() const {
    return deriv.is_clear;
}

Board_lifetime_stats Board::get_lifetime_stats() const {
    return lifetime_stats;
}

const Grid *Board::get_grid() const {
    return &foundation.grid;
}

void Board::load_ancestral_data_with_current_data() {

    ancestor_with_smallest_max_height.highest_height = deriv.highest_height;
    ancestor_with_smallest_max_height.second_lowest_height = deriv.second_lowest_height;
    ancestor_with_smallest_max_height.good_trench_status = deriv.has_good_trench_status;
}

bool Board::add_junk(int pos, int count){

    const bool is_game_over = foundation.add_junk(pos, count);
    if(!is_game_over){
        update_secondary_cache();
    }
    return is_game_over;
}


bool Board::at(size_t row, size_t col) const {
    return foundation.grid.at(row, col);
}



bool Board::is_promising() const {

    static constexpr int max_acceptable_holes_above_anc = 0;
    static constexpr int max_acceptable_height_increase = 3;

    const Ancestor_data& ancestor = ancestor_with_smallest_max_height;

    bool added_needless_trench = ancestor.good_trench_status && !deriv.has_good_trench_status;

    if(deriv.highest_height - ancestor.highest_height > max_acceptable_height_increase){
        return false;
    }
    if(added_needless_trench){
        return true;
    }

    int hole_leeway = (ancestor.highest_height == ancestor.second_lowest_height) ? 1 : 0;
    int holes_above_anc_max = num_holes_above_height(ancestor.highest_height + hole_leeway);
    if(holes_above_anc_max > max_acceptable_holes_above_anc){
        return false;
    }

    return true;

}

int Board::num_holes_above_height(int height) const {

    int found = 0;
    for(int col_x = 0; col_x < BoardSize::c_cols; ++col_x){
        // Would still be correct if it was height_map[col_x] - 1,
        for(int row_x = height; row_x <= foundation.height_map[col_x] - 2; ++row_x){
            if(!at(row_x, col_x)){
                ++found;
            }
        }
    }
    return found;
}


void Board::update_secondary_cache() {
    deriv.update(foundation);
    if(deriv.highest_height < ancestor_with_smallest_max_height.highest_height){
        load_ancestral_data_with_current_data();
    }
}

void Board::update_lifetime_cache(int num_rows_cleared_just_now){

    ++lifetime_stats.num_blocks_placed;
    if(num_rows_cleared_just_now > 0){
        ++lifetime_stats.num_placements_that_cleared_rows;
        if(num_rows_cleared_just_now == 4){
            ++lifetime_stats.num_tetrises;
        }
        else{
            ++lifetime_stats.num_non_tetrises;
        }
    }
    if(is_clear()){
        ++lifetime_stats.num_all_clears;
    }
    lifetime_stats.max_height_exp_moving_average =
        (0.5 * deriv.highest_height) +
        (0.5 * lifetime_stats.max_height_exp_moving_average);

}
