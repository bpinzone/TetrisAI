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

// Requires: This is a valid placement (nothing will go out of bounds)
// May or may not create holes.
// Returns true iff this move does NOT result in a game over.
bool State::place_block(const Block& b, Placement p){

    static const int c_cells_per_block = 4;

    const CH_maps& ch_map = b.maps[p.rotation];
    const int contour_size = ch_map.contour.size();

    int min_row_x_affected = c_rows - 1;
    int max_row_x_affected = 0;

    const int left_bottom_row = get_row_after_drop(b, p);
    for(int contour_x = 0; contour_x < contour_size; ++contour_x){
        const int col = p.column + contour_x;
        // absolute rows
        const int start_fill_row = left_bottom_row + ch_map.contour[contour_x];
        const int end_fill_row = start_fill_row + ch_map.height[contour_x];

        min_row_x_affected = min(min_row_x_affected, start_fill_row);
        max_row_x_affected = max(max_row_x_affected, end_fill_row - 1);

        if(max_row_x_affected >= c_rows){
            return false;
        }

        height_map[col] = end_fill_row;
        for(int row = start_fill_row; row < end_fill_row; ++row){
            at(row, col) = true;
        }

    }

    num_filled += c_cells_per_block;

    num_rows_cleared_on_last_place = 0;
    for(int row = max_row_x_affected; row >= min_row_x_affected; --row){
        if(is_row_full(row)){
            ++num_rows_cleared_on_last_place;
            clear_row(row);
        }
    }

    if(num_rows_cleared_on_last_place == 4){
        ++tetris_count;
    }

    can_hold = true;
    return true;
}

int State::get_num_holes() const {

    // TODO: Cache this accumulation.
    const int perfect_board_cell_count = accumulate(
        cbegin(height_map), cend(height_map), 0);

    return static_cast<int>(perfect_board_cell_count - num_filled);
}

bool State::get_can_hold() const {
    return can_hold;
}

const Block* State::hold(const Block& b){
    const Block* old_hold = current_hold;
    current_hold = &b;
    can_hold = false;
    return old_hold;
}

const Block* State::get_hold() const {
    return current_hold;
}

size_t State::populate_placements(
        const Block& b,
        std::array<Placement, c_worst_case_num_placements>::iterator dest) const{

    size_t size = 0;

    for(int rot_x = 0; rot_x < b.maps.size(); ++rot_x){
        const int max_valid_col = c_cols - b.maps[rot_x].contour.size();
        for(int col = 0; col <= max_valid_col; ++col){
            *dest = {rot_x, col};
            ++dest;
            ++size;
        }
    }
    return size;
}

bool State::has_higher_utility_than(const State& other) const {

    if(is_worst_board != other.is_worst_board){
        return !is_worst_board;
    }

    // You are in tetris mode if you are here or less in height.
    static const int c_max_tetris_mode_height = 15;

    // === Fundamental Priorities ===
    // Trench Punishment
    const int this_trenches = get_num_trenches();
    const int other_trenches = other.get_num_trenches();
    const bool this_has_more_than_1_trench = this_trenches > 1;
    const bool other_has_more_than_1_trench = other_trenches > 1;
    if(this_has_more_than_1_trench != other_has_more_than_1_trench){
        return !this_has_more_than_1_trench;
    }

    // Holes
    {
        const int this_holes = get_num_holes();
        const int other_holes = other.get_num_holes();
        if(this_holes != other_holes){
            return this_holes < other_holes;
        }
    }

    // Helper, compute heights
    const auto [this_min_height, this_max_height] = get_min_max_height();
    const int16_t this_height_diff = this_max_height - this_min_height;
    const auto [other_min_height, other_max_height] = other.get_min_max_height();
    const int16_t other_height_diff = other_max_height - other_min_height;

    // Tetris mode.
    const bool this_in_tetris_mode = this_max_height <= c_max_tetris_mode_height;
    const bool other_in_tetris_mode = other_max_height <= c_max_tetris_mode_height;
    if(this_in_tetris_mode != other_in_tetris_mode){
        return this_in_tetris_mode;
    }

    if(this_in_tetris_mode){
        assert(other_in_tetris_mode);
        // Do tetris mode comparison.

        // Get the tetrises
        if(tetris_count != other.tetris_count){
            return tetris_count > other.tetris_count;
        }

        // Become tetris-able.
        // Approximates tetris-ability in the sense that we assume there are no holes.
        // Recall: Getting rid of holes is the first priority.
        // If we have made it this far in comparison, we probably don't have many holes.
        const bool is_this_tetrisable =
            this_trenches == 1 && is_rest_board_4_above_single_trench();

        const bool is_other_tetrisable =
            other_trenches == 1 && other.is_rest_board_4_above_single_trench();

        if(is_this_tetrisable != is_other_tetrisable){
            return is_this_tetrisable;
        }

        // Now, both boards are in the same one of these two scenarios:
        // 1. We don't forsee a tetrisable future.
        // 2. We can tetris.

        // Make the 2nd shortest column as large as possible.
        // Encourges alg to build a solid mass of blocks, but not clear rows,
        // in order to get to the point where we can forsee being tetris-able.
        const int16_t this_second_height = get_height_of_second_lowest();
        const int16_t other_second_height = other.get_height_of_second_lowest();

        if(this_second_height != other_second_height){
            return this_second_height > other_second_height;
        }

        return this_height_diff < other_height_diff;

        // TODO: Maybe consider punishing clearing less than 3 rows at once.
    }
    else{
        assert(!this_in_tetris_mode);
        assert(!other_in_tetris_mode);
        // Do non-tetris mode comparison.

        if(this_trenches != other_trenches){
            return this_trenches < other_trenches;
        }

        if(num_filled != other.num_filled){
            return num_filled < other.num_filled;
        }

        return this_height_diff < other_height_diff;
    }


}

State::Board_t::reference State::at(size_t row, size_t col){

    size_t idx = c_size - 1 - ((row * c_cols) + col);
    return board[idx];
}

bool State::at(size_t row, size_t col) const {
    size_t idx = c_size - 1 - ((row * c_cols) + col);
    return board[idx];
}

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

void State::assert_cache_correct() const {

    assert(num_filled == board.count());
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
}

void State::clear_row(int deleted_row) {

    Board_t board_shifted_down = board << c_cols;

    Board_t below_del_row_mask;
    below_del_row_mask.set();
    below_del_row_mask <<= (c_cols * (c_rows - deleted_row));

    Board_t above_including_del_row_mask{~below_del_row_mask};

    for(int col_x = 0; col_x < c_cols; ++col_x){
        height_map[col_x] -= get_height_map_reduction(deleted_row, col_x);
    }

    board =
        (board & below_del_row_mask) |
        (board_shifted_down & above_including_del_row_mask);

    num_filled -= c_cols;
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

bool State::is_row_full(int row) const {

    for(int col = 0; col < c_cols; ++col){
        if(!at(row, col)){
            return false;
        }
    }
    return true;
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

int State::get_num_trenches() const {

    // Trench count
    int num_trenches = 0;
    if(height_map[1] - height_map[0] >= 3){
        ++num_trenches;
        // TODO: differentiate between punishing 3+, and having a 4+ trench.
        trench_col = 0;
    }
    if(height_map[c_cols - 2] - height_map[c_cols - 1] >= 3){
        ++num_trenches;
        // TODO: differentiate between punishing 3+, and having a 4+ trench.
        trench_col = c_cols - 1;
    }
    for(int col_x = 1; col_x < c_cols - 2; ++col_x){
        const int middle_height = height_map[col_x];
        const int left_height = height_map[col_x - 1];
        const int right_height = height_map[col_x + 1];
        if(left_height - middle_height >= 3 && right_height -middle_height >= 3){
            ++num_trenches;
            // TODO: differentiate between punishing 3+, and having a 4+ trench.
            trench_col = col_x;
        }
    }

    return num_trenches;
}

pair<int16_t, int16_t> State::get_min_max_height() const {

    const auto& [min_height_it, max_height_it] =
        minmax_element(cbegin(height_map), cend(height_map));

    return {*min_height_it, *max_height_it};
}


bool State::is_rest_board_4_above_single_trench() const {

    const int min_passing_height = height_map[trench_col] + 4;
    for(int col_x = 0; col_x < c_cols; ++col_x){
        if(height_map[col_x] < min_passing_height
                && col_x != trench_col){
            return false;
        }
    }
    return true;
}

int16_t State::get_height_of_second_lowest() const {

    int16_t lowest = c_rows;
    int16_t second_lowest = c_rows;

    for(const auto& height : height_map){

        if(height <= lowest){
            second_lowest = lowest;
            lowest = height;
        }
        else{
            second_lowest = min(height, second_lowest);
        }
    }
    return second_lowest;
}