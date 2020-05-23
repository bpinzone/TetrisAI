#include "board.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <utility>
#include <numeric>

using namespace std;

State State::worst_state;

/*
Example:

XX
 XX

contour: 0, -1, -1
height: 1, 2, 1
*/

const Block Block::Cyan {"Cyan", {
    // // XXXX
    {
        {0, 0, 0, 0}, // Contour
        {1, 1, 1, 1} // Height
    },
    // X
    // X
    // X
    // X
    {
        {0}, // Contour
        {4} // Height
    }
}};

const Block Block::Blue {"Blue", {
    // XX
    // X
    // X
    { {0, 2}, {3, 1} },

    //  X
    //  X
    // XX
    { {0, 0}, {1, 3} },

    // X
    // XXX
    { {0, 0, 0}, {2, 1, 1}},

    //   X
    // XXX
    { {0, 0, 0}, {1, 1, 2}},
}};

const Block Block::Orange {"Orange", {
    // XX
    //  X
    //  X
    { {0, -2}, {1, 3} },

    // X
    // X
    // XX
    { {0, 0}, {3, 1} },

    // XXX
    // X
    { {0, 1, 1}, {2, 1, 1}},

    // XXX
    //   X
    { {0, 0, -1}, {1, 1, 2}},
}};

const Block Block::Green {"Green", {

    //  XX
    // XX
    { {0, 0, 1}, {1, 2, 1} },

    //  X
    // XX
    // X
    { {0, 1}, {2, 2} }
}};

const Block Block::Red {"Red", {
    // XX
    //  XX
    { {0, -1, -1}, {1, 2, 1} },

    //  X
    // XX
    // X
    { {0, 1}, {2, 2} }
}};

const Block Block::Yellow {"Yellow", {
    // XX
    // XX
    { {0, 0}, {2, 2} }
}};

const Block Block::Purple {"Purple", {
    //  X
    // XXX
    { {0, 0, 0}, {1, 2, 1} },

    // X
    // XX
    // X
    { {0, 1}, {3, 1} },

    // XXX
    //  X
    { {0, -1, 0}, {1, 2, 1}},

    //  X
    // XX
    //  X
    { {0, -1}, {1, 3}},
}};

ostream& operator<<(ostream& os, const State& s) {

    cout << "Holding: ";
    cout << (s.current_hold ? s.current_hold->name : "none") << endl;

    for(long row = State::c_rows - 1; row >= 0; --row){
        for(long col = 0; col < State::c_cols; ++col){
            os << (s.at(row, col) ? "X" : ".");
        }
        os << "\n";
    }

    return os;
}

// Return true iff this board is still promising.
bool State::place_block(const Block& b, Placement p){

    static const int c_cells_per_block = 4;
    static const int max_holes_allowed_generated_at_once = 1;

    const int old_num_holes = get_num_holes();

    const int left_bottom_row = get_row_after_drop(b, p);

    const CH_maps& ch_map = b.maps[p.rotation];
    const int contour_size = ch_map.contour.size();

    int min_row_x_affected = c_rows - 1;
    int max_row_x_affected = 0;

    for(int contour_x = 0; contour_x < contour_size; ++contour_x){
        const int col = p.column + contour_x;
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

    num_cells_filled += c_cells_per_block;

    int num_rows_cleared_just_now = 0;
    for(int row = max_row_x_affected; row >= min_row_x_affected; --row){
        if(is_row_full(row)){
            ++num_rows_cleared_just_now;
            clear_row(row);
        }
    }

    if(num_rows_cleared_just_now > 0){
        ++num_placements_that_cleared_rows;
        if(num_rows_cleared_just_now == 4){
            ++num_tetrises;
        }
    }

    if(get_num_holes() - old_num_holes > max_holes_allowed_generated_at_once){
        // Moves that create more than 2 holes immediatley pruned.
        // Save time by not updating cache.
        return false;
    }

    update_cache();
    assert_cache_correct();

    just_swapped = false;
    return true;
}

const Block* State::swap_block(const Block& b){
    const Block* old_hold = current_hold;
    current_hold = &b;
    just_swapped = true;
    return old_hold;
}

bool State::has_higher_utility_than(const State& other) const {

    if(is_worst_board != other.is_worst_board){
        return !is_worst_board;
    }

    // You are in tetris mode if you are here or less in height.
    // todo: experiment with this later.
    static const int c_max_tetris_mode_height = 12;

    // === Fundamental Priorities ===
    // Trench Punishment
    // TODO: this is inlined
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

    const int16_t this_height_diff = highest_height - lowest_height;
    const int16_t other_height_diff = other.highest_height - other.lowest_height;

    // Tetris mode.
    const bool this_in_tetris_mode = highest_height <= c_max_tetris_mode_height;
    const bool other_in_tetris_mode = other.highest_height <= c_max_tetris_mode_height;
    if(this_in_tetris_mode != other_in_tetris_mode){
        return this_in_tetris_mode;
    }

    if(this_in_tetris_mode){
        assert(other_in_tetris_mode);
        // Do tetris mode comparison.

        // Get the tetrises
        if(num_tetrises != other.num_tetrises){
            return num_tetrises > other.num_tetrises;
        }

        if(is_tetrisable != other.is_tetrisable){
            return is_tetrisable;
        }

        // Make the 2nd shortest column as large as possible.
        // Encourges alg to build a solid mass of blocks, but not clear rows,
        // in order to get to the point where we can forsee being tetris-able.
        if(second_lowest_height != other.second_lowest_height){
            return second_lowest_height > other.second_lowest_height;
        }

        return this_height_diff < other_height_diff;
    }
    else{
        assert(!this_in_tetris_mode);
        assert(!other_in_tetris_mode);
        // Do non-tetris mode comparison.

        if(num_trenches != other.num_trenches){
            return num_trenches < other.num_trenches;
        }

        if(num_cells_filled != other.num_cells_filled){
            return num_cells_filled < other.num_cells_filled;
        }

        return this_height_diff < other_height_diff;
    }
}

int State::get_num_holes() const {
    return static_cast<int>(perfect_num_cells_filled - num_cells_filled);
}

bool State::can_swap_block(const Block& b) const {
    if(&b == current_hold){
        return false;
    }
    return !just_swapped;
}

void State::print_diff_against(const State& new_other) const{

    State new_items_state;
    new_items_state.board = ~board & new_other.board;

    State old_items_state;
    old_items_state.board = board & new_other.board;

    cout << "Holding: ";
    cout << (new_other.current_hold ? new_other.current_hold->name : "none") << endl;

    for(long row = State::c_rows - 1; row >= 0; --row){
        for(long col = 0; col < State::c_cols; ++col){

            if(new_items_state.at(row, col)){
                cout << "@";
            }
            else if(old_items_state.at(row, col)){
                cout << "X";
            }
            else{
                cout << ".";
            }
        }
        cout << "\n";
    }
}

double State::get_tetris_percent() const {
    return static_cast<double>(num_tetrises) / num_placements_that_cleared_rows * 100;
}

const State& State::get_worst_state() {
    worst_state.is_worst_board = true;
    return worst_state;
}

void State::clear_row(int deleted_row) {

    Board_t board_shifted_down = board << c_cols;

    Board_t below_del_row_mask;
    below_del_row_mask.set();
    below_del_row_mask <<= (c_cols * (c_rows - deleted_row));

    Board_t above_including_del_row_mask{~below_del_row_mask};

    for(int col_x = 0; col_x < c_cols; ++col_x){
        int16_t reduction = get_height_map_reduction(deleted_row, col_x);
        height_map[col_x] -= reduction;
        perfect_num_cells_filled -= reduction;
    }

    board =
        (board & below_del_row_mask) |
        (board_shifted_down & above_including_del_row_mask);

    num_cells_filled -= c_cols;
}

State::Board_t::reference State::at(size_t row, size_t col){

    size_t idx = c_size - 1 - ((row * c_cols) + col);
    return board[idx];
}

bool State::at(size_t row, size_t col) const {
    size_t idx = c_size - 1 - ((row * c_cols) + col);
    return board[idx];
}

bool State::is_row_full(int row) const {

    for(int col = 0; col < c_cols; ++col){
        if(!at(row, col)){
            return false;
        }
    }
    return true;
}

int State::get_row_after_drop(const Block& b, Placement p) const {

    const auto& contour = b.maps[p.rotation].contour;
    int num_cols_to_inspect = contour.size();

    int max_row = height_map[p.column];
    assert(contour.front() == 0);

    for(int col_x = 1; col_x < num_cols_to_inspect; ++col_x){

        int board_col = p.column + col_x;
        int row = height_map[board_col] - contour[col_x];
        max_row = max(max_row, row);
    }
    return max_row;
}

bool State::contour_matches(const Block& b, Placement p) const {

    const int leftmost_abs_col = p.column;
    const int height_of_first_col = height_map[leftmost_abs_col];
    const auto& contour_map = b.maps[p.rotation].contour;

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
int16_t State::get_height_map_reduction(int deleted_row, int query_col) const {

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

void State::update_cache() {

    static const int impossibly_high_wall = c_rows + 5;
    static const int min_depth_considered_trench = 3;

    num_trenches = 0;

    lowest_height = c_cols;
    second_lowest_height = c_cols;
    highest_height = 0;

    int left_height = impossibly_high_wall;
    int middle_height = height_map[0];
    int right_height = height_map[1];

    int some_trench_height = 0;

    for(int col_x = 0; col_x < c_cols; ++col_x){

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

void State::assert_cache_correct() const {

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
