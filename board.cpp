#include "board.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <utility>
#include <numeric>

using namespace std;

static const double hold_cyan_reward = 3;
static const double multi_trench_penalty = 9999;

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
bool State::place_block(Block b, Placement p){

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

    for(int row = max_row_x_affected; row >= min_row_x_affected; --row){
        if(is_row_full(row)){
            clear_row(row);
        }
    }
    return true;
}

int State::get_num_holes() const {

    const int perfect_board_cell_count = accumulate(
        cbegin(height_map), cend(height_map), 0);

    return static_cast<int>(perfect_board_cell_count - board.count());
}

optional<Block> State::hold(Block b){
    optional<Block> old_hold = current_hold;
    current_hold = b;
    return old_hold;
}

optional<Block> State::get_hold() const {
    return current_hold;
}

State::Placements_t State::get_placements(Block b) const {

    Placements_t placements;
    for(int rot_x = 0; rot_x < b.maps.size(); ++rot_x){
        const int max_valid_col = c_cols - b.maps[rot_x].contour.size();
        for(int col = 0; col <= max_valid_col; ++col){
            placements.push_back({rot_x, col});
        }
    }
    return placements;
}

double State::get_utility() const {

    // Holes
    const int num_holes = get_num_holes();

    // Heights
    const auto& [min_height_it, max_height_it] =
        minmax_element(cbegin(height_map), cend(height_map));
    const int16_t board_max_height = *max_height_it;
    const int16_t board_min_height = *min_height_it;
    const int16_t height_difference = board_max_height - board_min_height;

    // Trench count
    int num_trenches = 0;
    if(height_map[1] - height_map[0] >= 3){
        ++num_trenches;
    }
    if(height_map[c_cols - 2] - height_map[c_cols - 1] >= 3){
        ++num_trenches;
    }
    for(int col_x = 1; col_x < c_cols - 2; ++col_x){
        const int middle_height = height_map[col_x];
        const int left_height = height_map[col_x - 1];
        const int right_height = height_map[col_x + 1];
        if(left_height - middle_height >= 3 && right_height -middle_height >= 3){
            ++num_trenches;
        }
    }
    int current_trench_penalty = num_trenches <= 1 ? 0 : multi_trench_penalty;


    int current_cyan_reward = 0;
    if(current_hold && current_hold->name == Block::Cyan.name){
        current_cyan_reward = hold_cyan_reward;
    }

    return -(100 * num_holes) - (height_difference * height_difference * height_difference) - current_trench_penalty + current_cyan_reward;
    return -(100 * num_holes) - (height_difference * height_difference * height_difference) + current_cyan_reward;

    return -(100 * num_holes) - (board_max_height * board_max_height) + current_cyan_reward;
    return -(num_holes + board_max_height) + current_cyan_reward;
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

void State::clear_row(int deleted_row) {

    Board_t board_shifted_down = board << c_cols;

    Board_t below_del_row_mask;
    below_del_row_mask.set();
    below_del_row_mask <<= (c_cols * (c_rows - deleted_row));

    Board_t above_including_del_row_mask{~below_del_row_mask};

    board =
        (board & below_del_row_mask) |
        (board_shifted_down & above_including_del_row_mask);

    transform(begin(height_map), end(height_map), begin(height_map),
        [](auto height){
            return height - 1;
    });
}

bool State::is_row_full(int row) const {

    Board_t row_mask;
    row_mask.set();
    row_mask <<= (c_rows - 1) * c_cols;
    row_mask >>= row * c_cols;

    Board_t masked_board = board & row_mask;

    return masked_board == row_mask;
}

bool State::contour_matches(Block b, Placement p) const {

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

int State::get_row_after_drop(Block b, Placement p) const {

    int num_cols_to_inspect =
        b.maps[p.rotation].contour.size();

    int max_row = height_map[p.column];
    assert(b.maps[p.rotation].contour.front() == 0);

    for(int col_x = 1; col_x < num_cols_to_inspect; ++col_x){

        int board_col = p.column + col_x;
        int row = height_map[board_col] -
            b.maps[p.rotation].contour[col_x];
        max_row = max(max_row, row);
    }
    return max_row;
}