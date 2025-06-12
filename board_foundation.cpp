#include "board_foundation.h"

Board_foundation::Board_foundation(bool has_colors) : grid(has_colors) {
    reset();
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