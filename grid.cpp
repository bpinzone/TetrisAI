#include "grid.h"

#include <iostream>
#include <stdexcept>

#include "color_grid.h"

Grid::Grid(bool has_colors){
    if(has_colors){
        color_grid = ColorGrid();
    }
}

void Grid::clear_row(size_t row){
    bit_grid.clear_row(row);
    if(color_grid){
        color_grid->clear_row(row);
    }
}

void Grid::set_at(size_t row, size_t col, bool filled, Color color){

    bit_grid.set_at(row, col, filled);

    if(color_grid){
        ColorCell cell;
        cell.state.is_filled = filled;
        cell.color = color;
        color_grid->set_at(row, col, cell);
    }

}

bool Grid::add_junk(int pos, int count){
    const bool game_over = bit_grid.add_junk(pos, count);

    if(color_grid){
        color_grid->add_junk(pos, count);
    }
    return game_over;
}

bool Grid::at(size_t row, size_t col) const {
    return bit_grid.get_at(row, col);
}

size_t Grid::count() const{
    return bit_grid.count();
}


void Grid::output_color_grid_to_stream(std::ostream& os) const {
    if(!color_grid){
        throw std::logic_error{"Cannot output color grid to stream because it is not enabled!"};
    }
    color_grid->output_to_stream(os);
}

bool Grid::is_column_clear(size_t col) const {
    return bit_grid.is_column_clear(col);
}