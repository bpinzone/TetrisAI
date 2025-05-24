#include "color_grid.h"

void ColorGrid::set_at(size_t row, size_t col, ColorCell cell){
    board[row][col] = cell;
}

void ColorGrid::clear_row(size_t deleted_row){

    for(size_t overwrite_row = deleted_row;
        overwrite_row < BoardSize::c_rows; ++overwrite_row){
        
        size_t source_row = overwrite_row + 1;
        if(source_row < BoardSize::c_rows){
            for(size_t col = 0; col < BoardSize::c_cols; ++col){
                board[overwrite_row][col] = board[source_row][col];
            }
        }
        else{
            for(size_t col = 0; col < BoardSize::c_cols; ++col){
                board[overwrite_row][col] = ColorCell{};
            }
        }
    }
}