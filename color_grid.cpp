#include "color_grid.h"
#include <iostream>
#include "block.h"

void ColorCellShade::output_to_stream(std::ostream& os) const {
    os << "(";
    os << "f:" << is_filled << ",";
    os << "g:" << is_ghost << ",";
    os << "a:" << is_about_to_be_cleared << ")";
}

void ColorCell::output_to_stream(std::ostream& os) const {
    os << "c:" << Block::color_to_char(color) << ",";
    os << "s:";
    state.output_to_stream(os);
}

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

void ColorGrid::output_to_stream(std::ostream& os) const {

    os << "rows: " << BoardSize::c_rows << "\n";
    os << "cols: " << BoardSize::c_cols << "\n";
    for(long row = 0; row < BoardSize::c_rows; ++row){
        for(long col = 0; col < BoardSize::c_cols; ++col){
            os << "row:" << row << ", col:" << col << ", data: ";
            board[row][col].output_to_stream(os);
            os << "\n";
        }
    }
}