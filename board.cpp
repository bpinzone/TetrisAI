#include "board.h"

#include "block.h"
#include "utility.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <utility>
#include <numeric>
#include <iostream>

using namespace std;

Board Board::worst_board;

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

    update_secondary_cache();

}

ostream& operator<<(ostream& os, const Board& s) {

    os << "Holding: ";
    os << (s.current_hold ? s.current_hold->name : "none") << endl;

    for(long row = Board::c_rows - 1; row >= 0; --row){
        for(long col = 0; col < Board::c_cols; ++col){
            os << (s.at(row, col) ? "X" : ".");
        }
        os << "\n";
    }

    os << "All clears: " << s.num_all_clears << endl;
    os << "Tetrises: " << s.num_tetrises << endl;

    return os;
}

// Return true iff this board is still promising.
bool Board::place_block(const Block& b, Placement p){

    assert_cache_correct();

    assert(!p.get_is_hold());
    const int old_num_holes = get_num_holes();

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

    // Don't update cache if we're not promising
    static const int max_holes_allowed_generated_at_once = 1;
    if(get_num_holes() - old_num_holes > max_holes_allowed_generated_at_once){
        // Moves that create more than 2 holes immediatley pruned.
        // Save time by not updating cache.
        return false;
    }

    update_secondary_cache();
    update_lifetime_cache(num_rows_cleared_just_now);
    assert_cache_correct();

    just_swapped = false;
    return true;
}

const Block* Board::swap_block(const Block& b){
    const Block* old_hold = current_hold;
    current_hold = &b;
    just_swapped = true;
    return old_hold;
}

// NOTE: Values for granularity? Hybrid?
bool Board::has_greater_utility_than(const Board& other) const {

    if(is_worst_board != other.is_worst_board){
        return !is_worst_board;
    }

    if(num_all_clears != other.num_all_clears){
        return num_all_clears > other.num_all_clears;
    }

    // You are in tetris mode if you are here or less in height.
    // todo: experiment with this later. How it relates to tetris percent.
    static const int c_max_tetris_mode_height = 6;
    static const int c_height_diff_punishment_thresh = 3;

    const bool this_in_tetris_mode = highest_height <= c_max_tetris_mode_height;
    const bool other_in_tetris_mode = other.highest_height <= c_max_tetris_mode_height;

    // === Fundamental Priorities ===
    if((num_trenches > 1) != (other.num_trenches > 1)){
        return num_trenches <= 1;
    }
    // Holes
    {
        const int this_holes = get_num_holes();
        const int other_holes = other.get_num_holes();
        if(this_holes != other_holes){
            return this_holes < other_holes;
        }
    }

    // Prefer to be in Tetris mode.
    if(this_in_tetris_mode != other_in_tetris_mode){
        return this_in_tetris_mode;
    }

    if(this_in_tetris_mode && other_in_tetris_mode){
        if(at_least_one_side_clear != other.at_least_one_side_clear){
            return at_least_one_side_clear;
        }
        if(num_non_tetrises != other.num_non_tetrises){
            return num_non_tetrises < other.num_non_tetrises;
        }
    }

    // Keep relatively even except for the one trench.
    const int this_receives_height_punishment = highest_height - second_lowest_height >= c_height_diff_punishment_thresh;
    const int other_receives_height_punishment = other.highest_height - other.second_lowest_height >= c_height_diff_punishment_thresh;
    if(this_receives_height_punishment != other_receives_height_punishment){
        return !this_receives_height_punishment;
    }

    if(this_in_tetris_mode){

        // Get the tetrises
        if(num_tetrises != other.num_tetrises){
            return num_tetrises > other.num_tetrises;
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

        return sum_of_squared_heights < other.sum_of_squared_heights;

    }
    else{

        if(num_trenches != other.num_trenches){
            return num_trenches < other.num_trenches;
        }

        if(num_cells_filled != other.num_cells_filled){
            return num_cells_filled < other.num_cells_filled;
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

void Board::print_diff_against(const Board& new_other) const{

    Board new_items_state;
    new_items_state.board = ~board & new_other.board;

    Board old_items_state;
    old_items_state.board = board & new_other.board;

    Output_manager::get_instance().get_board_os() << "Holding: ";
    Output_manager::get_instance().get_board_os() << (new_other.current_hold ? new_other.current_hold->name : "none") << endl;

    for(long row = Board::c_rows - 1; row >= 0; --row){
        for(long col = 0; col < Board::c_cols; ++col){
            if(new_items_state.at(row, col)){
                Output_manager::get_instance().get_board_os() << "@";
            }
            else if(old_items_state.at(row, col)){
                Output_manager::get_instance().get_board_os() << "X";
            }
            else{
                Output_manager::get_instance().get_board_os() << ".";
            }
        }
        Output_manager::get_instance().get_board_os() << "\n";
    }
}

int Board::get_num_blocks_placed() const {
    return num_blocks_placed;
}

double Board::get_tetris_percent() const {
    return static_cast<double>(num_tetrises) / num_blocks_placed * 100;
}

bool Board::has_more_cleared_rows_than(const Board& other) const {
    return num_placements_that_cleared_rows > other.num_placements_that_cleared_rows;
}

bool Board::is_clear() const {
    return num_cells_filled == 0;
}

const Board& Board::get_worst_board() {
    worst_board.is_worst_board = true;
    return worst_board;
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

bool Board::contour_matches(const Block& b, Placement p) const {

    const int leftmost_abs_col = p.get_column();
    const int height_of_first_col = height_map[leftmost_abs_col];
    const auto& contour_map = b.maps[p.get_rotation()].contour;

    for(int cols_checked = 0; cols_checked < contour_map.size(); ++cols_checked){
        const int abs_board_height = height_map[leftmost_abs_col + cols_checked];
        const int rel_board_height = abs_board_height - height_of_first_col;
        if(contour_map[cols_checked] != rel_board_height){
            return false;
        }
    }
    return true;
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

void Board::update_secondary_cache() {

    static const int impossibly_high_wall = c_rows + 5;
    static const int min_depth_considered_trench = 3;

    num_trenches = 0;
    at_least_one_side_clear = (height_map[0] == 0) || (height_map[c_cols - 1] == 0);
    lowest_height = c_cols;
    second_lowest_height = c_cols;
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
}

void Board::update_lifetime_cache(int num_rows_cleared_just_now){
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
    if(is_clear()){
        ++num_all_clears;
    }

}

void Board::assert_cache_correct() const {

    assert(num_cells_filled == board.count());
    for(int col_x = 0; col_x < c_cols; ++col_x){

        // Make sure rows above are empty.
        for(int row_x = height_map[col_x]; row_x < c_rows; ++row_x){
            assert(!at(row_x, col_x));
        }

        // Make sure height map has a cell filled in.
        if(height_map[col_x] > 0){
            assert(at(height_map[col_x] - 1, col_x));
        }
    }
    const int correct_perfect_board_cell_count = accumulate(
        cbegin(height_map), cend(height_map), 0);
    assert(perfect_num_cells_filled == correct_perfect_board_cell_count);
}
