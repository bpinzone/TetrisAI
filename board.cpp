#include "board.h"

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

Board::Board(istream& is){

    string label;
    is >> label;
    assert(label == "board");
    for(int row_x = static_cast<int>(c_rows - 1); row_x >= 0; --row_x){
        for(size_t col_x = 0; col_x < c_cols; ++col_x){
            char cell;
            is >> cell;
            at(static_cast<size_t>(row_x), col_x) = (cell == 'x');
        }
    }

    is >> label;
    assert(label == "in_hold");
    char hold;
    is >> hold;
    if(hold != '.'){
        current_hold = Block::char_to_block_ptr(hold);
    }

    is >> label;
    assert(label == "just_swapped");
    string just_swapped_str;
    is >> just_swapped_str;
    just_swapped = (just_swapped_str == "true");

    // Update things that cache does not do.
    for(size_t col_x = 0; col_x < c_cols; ++col_x){
        int height = compute_height(col_x);
        height_map[col_x] = height;
        perfect_num_cells_filled += height;
    }
    num_cells_filled = board.count();

    update_secondary_cache(0);
    load_ancestral_data_with_current_data();

}

ostream& operator<<(ostream& os, const Board& s) {

    os << "Holding: ";
    os << (s.current_hold ? s.current_hold->name : "none") << "\n";

    for(long row = Board::c_rows - 1; row >= 0; --row){
        for(long col = 0; col < Board::c_cols; ++col){
            os << (s.at(row, col) ? "X" : ".");
        }
        os << "\n";
    }

    os << "All clears: " << s.lifetime_stats.num_all_clears << "\n";
    os << "Tetrises: " << s.lifetime_stats.num_tetrises << "\n";

    return os;
}

// Return true iff this board is still promising.
bool Board::place_block(const Block& b, Placement p){

    assert(!p.get_is_hold());

    const int left_bottom_row = get_row_after_drop(b, p);

    const CH_maps& ch_map = b.maps[p.get_rotation()];
    const int contour_size = ch_map.contour.size();

    int min_row_x_affected = c_rows - 1;
    int max_row_x_affected = 0;

    for(int contour_x = 0; contour_x < contour_size; ++contour_x){
        const int col = p.get_column() + contour_x;
        const int abs_start_fill_row = left_bottom_row + ch_map.contour[contour_x];
        const int abs_end_fill_row = abs_start_fill_row + ch_map.height[contour_x];

        min_row_x_affected = min(min_row_x_affected, abs_start_fill_row);
        max_row_x_affected = max(max_row_x_affected, abs_end_fill_row - 1);

        if(max_row_x_affected >= c_rows){
            // NOTE: If we're here, this state is never touched again.
            // Because its game over.
            return false;
        }

        perfect_num_cells_filled -= height_map[col];
        height_map[col] = abs_end_fill_row;
        perfect_num_cells_filled += abs_end_fill_row;

        for(int row = abs_start_fill_row; row < abs_end_fill_row; ++row){
            at(row, col) = true;
        }
    }

    static const int c_cells_per_block = 4;
    num_cells_filled += c_cells_per_block;

    // Check for cleared rows
    int num_rows_cleared_just_now = 0;
    for(int row = max_row_x_affected; row >= min_row_x_affected; --row){
        if(is_row_full(row)){
            ++num_rows_cleared_just_now;
            clear_row(row);
        }
    }

    // must be called before is_promising.
    update_secondary_cache(num_rows_cleared_just_now);

    if(!is_promising()){
        return false;
    }

    update_lifetime_cache(num_rows_cleared_just_now);
    just_swapped = false;
    return true;
}

const Block* Board::swap_block(const Block& b){
    const Block* old_hold = current_hold;
    current_hold = &b;
    just_swapped = true;
    return old_hold;
}

void Board::set_lifetime_stats(const Board_lifetime_stats& new_lifetime_stats){
    lifetime_stats = new_lifetime_stats;
}

bool Board::has_greater_utility_than(const Board& other) const {

    ++gs_num_comparisons;

    if(lifetime_stats.num_all_clears != other.lifetime_stats.num_all_clears){
        return lifetime_stats.num_all_clears > other.lifetime_stats.num_all_clears;
    }

    // You are in tetris mode if you are here or less in height.
    // static const int c_max_tetris_mode_height = 6;
    static const int c_max_tetris_mode_height = 0;
    static const int c_height_diff_punishment_thresh = 3;

    const bool this_in_tetris_mode = highest_height <= c_max_tetris_mode_height;
    const bool other_in_tetris_mode = other.highest_height <= c_max_tetris_mode_height;

    // === Fundamental Priorities ===
    if(has_good_trench_status() != other.has_good_trench_status()){
        return has_good_trench_status();
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
        if(at_least_one_side_clear != other.at_least_one_side_clear){
            return at_least_one_side_clear;
        }
        if(lifetime_stats.num_non_tetrises != other.lifetime_stats.num_non_tetrises){
            return lifetime_stats.num_non_tetrises < other.lifetime_stats.num_non_tetrises;
        }
    }

    // Keep relatively even except for the one trench.
    const bool this_receives_height_punishment = highest_height - second_lowest_height >= c_height_diff_punishment_thresh;
    const bool other_receives_height_punishment = other.highest_height - other.second_lowest_height >= c_height_diff_punishment_thresh;
    if(this_receives_height_punishment != other_receives_height_punishment){
        return !this_receives_height_punishment;
    }

    if(num_trenches != other.num_trenches){
        return num_trenches < other.num_trenches;
    }

    if(this_in_tetris_mode){

        // Get the tetrises
        if(lifetime_stats.num_tetrises != other.lifetime_stats.num_tetrises){
            return lifetime_stats.num_tetrises > other.lifetime_stats.num_tetrises;
        }

        // Become tetrisable
        if(is_tetrisable != other.is_tetrisable){
            return is_tetrisable;
        }

        // Build up
        // Make the 2nd shortest column as large as possible.
        // Encourges alg to build a solid mass of blocks, but not clear rows,
        // in order to get to the point where we can forsee being tetris-able.
        if(second_lowest_height != other.second_lowest_height){
            return second_lowest_height > other.second_lowest_height;
        }

        if(sum_of_squared_heights != other.sum_of_squared_heights){
            return sum_of_squared_heights < other.sum_of_squared_heights;
        }

        if (lifetime_stats.max_height_exp_moving_average == other.lifetime_stats.max_height_exp_moving_average && board != other.board){
            std::cout << "---------------------------------------------------" << std::endl;
            std::cout << "These 2 boards compared equal: " << std::endl;
            std::cout << "Board 1:" << std::endl;
            std::cout << *this << std::endl << std::endl;
            std::cout << "Board 2:" << std::endl;
            std::cout << other << std::endl;
            std::cout << "---------------------------------------------------" << std::endl;
        }

        return lifetime_stats.max_height_exp_moving_average < other.lifetime_stats.max_height_exp_moving_average;

    }
    else{

        if(num_cells_filled != other.num_cells_filled){
            return num_cells_filled < other.num_cells_filled;
        }

        if (lifetime_stats.max_height_exp_moving_average != other.lifetime_stats.max_height_exp_moving_average){
            return lifetime_stats.max_height_exp_moving_average < other.lifetime_stats.max_height_exp_moving_average;

        }

        if (sum_of_squared_heights == other.sum_of_squared_heights && board != other.board){
            std::cout << "---------------------------------------------------" << std::endl;
            std::cout << "These 2 boards compared equal: " << std::endl;
            std::cout << "Board 1:" << std::endl;
            std::cout << *this << std::endl << std::endl;
            std::cout << "Board 2:" << std::endl;
            std::cout << other << std::endl;
            std::cout << "---------------------------------------------------" << std::endl;
        }

        return sum_of_squared_heights < other.sum_of_squared_heights;
    }

}

int Board::get_num_holes() const {
    return perfect_num_cells_filled - num_cells_filled;
}

bool Board::can_swap_block(const Block& b) const {
    if(&b == current_hold){
        return false;
    }
    return !just_swapped;
}

bool Board::is_holding_some_block() const {
    return current_hold;
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
    return num_cells_filled == 0;
}

Board_lifetime_stats Board::get_lifetime_stats() const {
    return lifetime_stats;
}

void Board::clear_row(int deleted_row) {

    Grid_t board_shifted_down = board << c_cols;

    Grid_t below_del_row_mask;
    below_del_row_mask.set();
    below_del_row_mask <<= (c_cols * (c_rows - deleted_row));

    Grid_t above_including_del_row_mask{~below_del_row_mask};

    for(int col_x = 0; col_x < c_cols; ++col_x){
        int reduction = get_height_map_reduction(deleted_row, col_x);
        height_map[col_x] -= reduction;
        perfect_num_cells_filled -= reduction;
    }

    board =
        (board & below_del_row_mask) |
        (board_shifted_down & above_including_del_row_mask);

    num_cells_filled -= c_cols;
}

Board::Grid_t::reference Board::at(size_t row, size_t col){

    size_t idx = c_size - 1 - ((row * c_cols) + col);
    return board[idx];
}

void Board::load_ancestral_data_with_current_data() {

    ancestor_with_smallest_max_height.highest_height = highest_height;
    ancestor_with_smallest_max_height.second_lowest_height = second_lowest_height;
    ancestor_with_smallest_max_height.good_trench_status = has_good_trench_status();
}


bool Board::at(size_t row, size_t col) const {
    size_t idx = c_size - 1 - ((row * c_cols) + col);
    return board[idx];
}

bool Board::is_row_full(int row) const {

    for(int col = 0; col < c_cols; ++col){
        if(!at(row, col)){
            return false;
        }
    }
    return true;
}

// Compute height based on "board" only.
int Board::compute_height(size_t col_x) const {
    int height = 0;
    for(size_t row_x = 0; row_x < c_rows; ++row_x){
        if(at(row_x, col_x)){
            height = row_x + 1;
        }
    }
    return height;
}

bool Board::is_promising() const {

    return true;

    static constexpr int max_acceptable_holes_above_anc = 0;
    static constexpr int max_acceptable_height_increase = 3;

    const Ancestor_data& ancestor = ancestor_with_smallest_max_height;

    bool added_needless_trench = ancestor.good_trench_status && !has_good_trench_status();

    // if(highest_height - ancestor.highest_height > max_acceptable_height_increase){
    //     return false;
    // }
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

bool Board::has_good_trench_status() const {
    return num_trenches <= 1;
}

int Board::num_holes_above_height(int height) const {

    int found = 0;
    for(int col_x = 0; col_x < c_cols; ++col_x){
        // Would still be correct if it was height_map[col_x] - 1,
        for(int row_x = height; row_x <= height_map[col_x] - 2; ++row_x){
            if(!at(row_x, col_x)){
                ++found;
            }
        }
    }
    return found;
}

int Board::get_row_after_drop(const Block& b, Placement p) const {

    const auto& contour = b.maps[p.get_rotation()].contour;
    int num_cols_to_inspect = contour.size();

    int max_row = height_map[p.get_column()];
    assert(contour.front() == 0);

    for(int col_x = 1; col_x < num_cols_to_inspect; ++col_x){
        int board_col = p.get_column() + col_x;
        int row = height_map[board_col] - contour[col_x];
        max_row = max(max_row, row);
    }
    return max_row;
}

// Given a row is being deleted, how many to subtract from the height map
// of query col. (Maybe holes will become exposed.)
int Board::get_height_map_reduction(int deleted_row, int query_col) const {

    int reductions = 1;
    // The deleted row IS NOT THE SURFACE at this column.
    bool is_surface = deleted_row == height_map[query_col] - 1;
    if(is_surface){
        for(int row_x = deleted_row - 1;
                row_x >= 0 && !at(row_x, query_col); --row_x){
            ++reductions;
        }
    }
    return reductions;
}

void Board::update_secondary_cache(int num_rows_cleared_just_now) {

    static constexpr int impossibly_high_wall = c_rows + 5;
    static constexpr int min_depth_considered_trench = 3;

    num_trenches = 0;
    at_least_one_side_clear = (height_map[0] == 0) || (height_map[c_cols - 1] == 0);
    lowest_height = c_rows;
    second_lowest_height = c_rows;
    highest_height = 0;
    sum_of_squared_heights = 0;

    int left_height = impossibly_high_wall;
    int middle_height = height_map[0];
    int right_height = height_map[1];

    int some_trench_height = 0;

    for(int col_x = 0; col_x < c_cols; ++col_x){

        sum_of_squared_heights += middle_height * middle_height;
        second_lowest_height = middle_height <= lowest_height ? lowest_height : min(second_lowest_height, middle_height);
        lowest_height = min(lowest_height, middle_height);
        highest_height = max(highest_height, middle_height);

        // count and keep track of a trench.
        if(left_height - middle_height >= min_depth_considered_trench
                && right_height - middle_height >= min_depth_considered_trench){
            ++num_trenches;
            some_trench_height = middle_height;
        }

        left_height = middle_height;
        middle_height = right_height;
        right_height = (col_x == c_cols - 2) ? impossibly_high_wall : height_map[col_x + 2];
    }

    is_tetrisable =
        num_trenches == 1
        && lowest_height == some_trench_height
        && second_lowest_height >= some_trench_height + 4;

    if(highest_height < ancestor_with_smallest_max_height.highest_height){
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
        (0.5 * highest_height) +
        (0.5 * lifetime_stats.max_height_exp_moving_average);

}
