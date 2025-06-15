#include "board_foundation.h"
#include <iostream>

using std::string;

Board_foundation::Board_foundation(bool has_colors) : grid(has_colors) {
    reset();
}

Board_foundation::Board_foundation(bool has_colors, std::istream& is)
    : Board_foundation(has_colors) {

    string label;
    is >> label;
    assert(label == "board");
    for(int row_x = static_cast<int>(BoardSize::c_rows - 1); row_x >= 0; --row_x){
        for(size_t col_x = 0; col_x < BoardSize::c_cols; ++col_x){
            char cell;
            is >> cell;
            Color unknown_color = Color::Blue;
            grid.set_at(static_cast<size_t>(row_x), col_x,
                (cell == 'x'),
                unknown_color);
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
    for(size_t col_x = 0; col_x < BoardSize::c_cols; ++col_x){
        int height = compute_height(col_x);
        height_map[col_x] = height;
        perfect_num_cells_filled += height;
    }
    num_cells_filled = grid.count();
}

std::ostream& operator<<(std::ostream& os, const Board_foundation& foundation){

    os << "Holding: ";
    os << (foundation.current_hold ?
            Block::name_to_full_name(foundation.current_hold->name)
            : "none");
    os << "\n";

    for(long row = BoardSize::c_rows - 1; row >= 0; --row){
        for(long col = 0; col < BoardSize::c_cols; ++col){
            os << (foundation.grid.at(row, col) ? "X" : ".");
        }
        os << "\n";
    }

    return os;
}

void Board_foundation::reset() {
    current_hold = nullptr;
    just_swapped = false;

    height_map = std::array<int, BoardSize::c_cols>{0};
    num_cells_filled = 0;
    perfect_num_cells_filled = 0;

}

void Board_foundation::clear_row(int deleted_row){

    for(int col_x = 0; col_x < BoardSize::c_cols; ++col_x){
        int reduction = get_height_map_reduction(deleted_row, col_x);
        height_map[col_x] -= reduction;
        perfect_num_cells_filled -= reduction;
    }

    grid.clear_row(deleted_row);

    num_cells_filled -= BoardSize::c_cols;

}

bool Board_foundation::place_block_no_clearing(const Block& b, Placement p,
        int *min_row_x_affected, int *max_row_x_affected)
{
    const int left_bottom_row = get_row_after_drop(b, p);

    const CH_maps& ch_map = b.maps[p.get_rotation()];
    const int contour_size = ch_map.contour.size();

    *min_row_x_affected = BoardSize::c_rows - 1;
    *max_row_x_affected = 0;

    for(int contour_x = 0; contour_x < contour_size; ++contour_x){
        const int col = p.get_column() + contour_x;
        const int abs_start_fill_row = left_bottom_row + ch_map.contour[contour_x];
        const int abs_end_fill_row = abs_start_fill_row + ch_map.height[contour_x];

        *min_row_x_affected = std::min(*min_row_x_affected, abs_start_fill_row);
        *max_row_x_affected = std::max(*max_row_x_affected, abs_end_fill_row - 1);

        if(*max_row_x_affected >= BoardSize::c_rows){
            // NOTE: If we're here, this state is never touched again.
            // Because its game over.
            // critical TODO: how have we not caught this? don't we need to check if this placement allows us to clear a row and hence survive?
            // critical TODO: how have we not caught this? don't we need to check if this placement allows us to clear a row and hence survive?
            // critical TODO: how have we not caught this? don't we need to check if this placement allows us to clear a row and hence survive?
            return true;
        }

        perfect_num_cells_filled -= height_map[col];
        height_map[col] = abs_end_fill_row;
        perfect_num_cells_filled += abs_end_fill_row;

        for(int row = abs_start_fill_row; row < abs_end_fill_row; ++row){
            grid.set_at(static_cast<size_t>(row),
                static_cast<size_t>(col),
                true,
                b.color
            );
        }
    }

    static const int c_cells_per_block = 4;
    num_cells_filled += c_cells_per_block;

    return false;
}


int Board_foundation::check_and_clear_rows(int min_row_x_affected, int max_row_x_affected){

    int num_rows_cleared_just_now = 0;
    for(int row = max_row_x_affected; row >= min_row_x_affected; --row){
        if(is_row_full(row)){
            ++num_rows_cleared_just_now;
            clear_row(row);
        }
    }
    return num_rows_cleared_just_now;
}

// Given a row is being deleted, how many to subtract from the height map
// of query col. (Maybe holes will become exposed.)
int Board_foundation::get_height_map_reduction(int deleted_row, int query_col) const {

    int reductions = 1;
    // The deleted row IS NOT THE SURFACE at this column.
    bool is_surface = deleted_row == height_map[query_col] - 1;
    if(is_surface){
        for(int row_x = deleted_row - 1;
                row_x >= 0 && !grid.at(row_x, query_col); --row_x){
            ++reductions;
        }
    }
    return reductions;
}


int Board_foundation::compute_height(size_t col_x) const {
    int height = 0;
    for(size_t row_x = 0; row_x < BoardSize::c_rows; ++row_x){
        if(grid.at(row_x, col_x)){
            height = row_x + 1;
        }
    }
    return height;
}

bool Board_foundation::is_row_full(int row) const {
    for(int col = 0; col < BoardSize::c_cols; ++col){
        if(!grid.at(row, col)){
            return false;
        }
    }
    return true;

}

// Given a block and placement, drop the block:
// return the row idx of the left-bottom most cell of the block.
int Board_foundation::get_row_after_drop(const Block& b, Placement p) const {


    const auto& contour = b.maps[p.get_rotation()].contour;
    int num_cols_to_inspect = contour.size();

    int max_row = height_map[p.get_column()];
    assert(contour.front() == 0);

    for(int col_x = 1; col_x < num_cols_to_inspect; ++col_x){
        int board_col = p.get_column() + col_x;
        int row = height_map[board_col] - contour[col_x];
        max_row = std::max(max_row, row);
    }
    return max_row;


}