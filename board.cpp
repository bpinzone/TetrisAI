#include "board.h"

#include <algorithm>
#include <cassert>
#include <iterator>

using namespace std;
/*
Example:

XX
 XX

contour: 0, -1, -1
height: 1, 2, 1
*/

const Block Block::Cyan {{
    // XXXX
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

const Block Block::Blue {{
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

const Block Block::Orange {{
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

const Block Block::Green {{

    //  XX
    // XX
    { {0, 0, 1}, {1, 2, 1} },

    //  X
    // XX
    // X
    { {0, 1}, {2, 2} }
}};

const Block Block::Red {{
    // XX
    //  XX
    { {0, -1, -1}, {1, 2, 1} },

    //  X
    // XX
    // X
    { {0, 1}, {2, 2} }
}};

const Block Block::Yellow {{
    // XX
    // XX
    { {0, 0}, {2, 2} }
}};

// Requires: This is a valid placement (nothing will go out of bounds)
// May or may not create holes.
void State::place_block(Block b, Placement p){

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

        height_map[col] = end_fill_row;
        for(int row = start_fill_row; row < end_fill_row; ++row){
            at(row, col) = true;
        }

    }

    for(int row = min_row_x_affected; row <= max_row_x_affected; ++row){
        if(is_row_full(row)){
            remove_row(row);
        }
    }
}

optional<Block> State::hold(Block b){
    optional<Block> old_hold = current_hold;
    current_hold = b;
    return old_hold;
}

optional<Block> State::get_hold() const {
    return current_hold;
}

Placements State::get_placements(Block b) const {

    Placements placements;
    for(int rot_x = 0; rot_x < b.maps.size(); ++rot_x){
        const int max_valid_col = c_cols - b.maps[rot_x].contour.size();
        for(int col = 0; col <= max_valid_col; ++col){
            // We have a valid placement
            placements.hole.push_back({rot_x, col});
        }
    }
    return placements;
}

int State::get_utility() const {
    // potential optimization, cache max height.
    return c_rows -
        *max_element(cbegin(height_map), cend(height_map));
}

State::Board_t::reference State::at(size_t row, size_t col){
    return board[(row * c_cols) + col];
}

bool State::at(size_t row, size_t col) const {
    return board[(row * c_cols) + col];
}

ostream& operator<<(ostream& os, const State& s) {

    for(long row = State::c_rows - 1; row >= 0; --row){
        for(long col = 0; col < State::c_cols; ++col){
            os << (s.at(row, col) ? "X" : ".");
        }
        os << "\n";
    }

    return os;
}


void State::remove_row(int deleted_row) {

    Board_t board_shifted_down = board << c_cols;

    Board_t below_del_row_mask;
    below_del_row_mask.set();
    below_del_row_mask << (c_cols * (c_rows - deleted_row));

    Board_t above_including_del_row_mask{~below_del_row_mask};

    board =
        (board & below_del_row_mask) |
        (board_shifted_down & above_including_del_row_mask);
}

bool State::is_row_full(int row) const {

    Board_t row_mask;
    row_mask.set();
    row_mask <<= (c_rows - 1) * c_cols;
    row_mask >>= row * c_cols;

    Board_t masked_board = board & row_mask;

    return masked_board == row_mask;
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