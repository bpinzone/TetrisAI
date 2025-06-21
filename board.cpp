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
    os << s.lifetime_stats;
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

    lifetime_stats.update(foundation, deriv, num_rows_cleared_just_now);
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

    #define PREFER_LESS(member) do { \
        const auto &this_member = this->member; \
        const auto &other_member = other.member; \
        if(this_member != other_member){ \
            return this_member < other_member; \
        }\
    } while(false);

    #define PREFER_MORE(member) do { \
        const auto &this_member = this->member; \
        const auto &other_member = other.member; \
        if(this_member != other_member){ \
            return this_member > other_member; \
        }\
    } while(false);

    #define PREFER_FALSE(member) do { \
        const auto &this_member = this->member; \
        const auto &other_member = other.member; \
        if(this_member != other_member){ \
            return !this_member; \
        }\
    } while(false);

    #define PREFER_TRUE(member) do { \
        const auto &this_member = this->member; \
        const auto &other_member = other.member; \
        if(this_member != other_member){ \
            return this_member; \
        }\
    } while(false);

    ++gs_num_comparisons;

    // PREFER_MORE(lifetime_stats.num_all_clears);

    // === Fundamental Priorities ===
    PREFER_TRUE(deriv.has_good_trench_status);
    PREFER_LESS(deriv.num_holes);
    PREFER_TRUE(deriv.in_tetris_mode);


    if(deriv.in_tetris_mode && other.deriv.in_tetris_mode){
        PREFER_TRUE(deriv.at_least_one_side_clear);
        PREFER_LESS(lifetime_stats.num_non_tetrises);
    }

    // Keep relatively even except for the one trench.
    PREFER_FALSE(deriv.receives_height_punishment);

    if(deriv.in_tetris_mode){

        PREFER_MORE(lifetime_stats.num_tetrises);
        PREFER_TRUE(deriv.is_tetrisable);
        // Build up
        // Make the 2nd shortest column as large as possible.
        // Encourges alg to build a solid mass of blocks, but not clear rows,
        // in order to get to the point where we can forsee being tetris-able.
        PREFER_MORE(deriv.second_lowest_height);
        PREFER_LESS(deriv.sum_of_squared_heights);
        PREFER_LESS(lifetime_stats.max_height_exp_moving_average);
        return false;
    }
    else{
        PREFER_LESS(deriv.num_trenches);
        PREFER_LESS(foundation.num_cells_filled);
        PREFER_LESS(deriv.sum_of_squared_heights);
        PREFER_LESS(lifetime_stats.max_height_exp_moving_average);
        return false;
    }

}

bool Board::can_swap_block(const Block& b) const {
    if(&b == foundation.current_hold){
        return false;
    }
    return !foundation.just_swapped;
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

