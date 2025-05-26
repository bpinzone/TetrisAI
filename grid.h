#ifndef GRID_H
#define GRID_H

#include <optional>

#include "bit_grid.h"
#include "color_grid.h"

// Class to operate on a grid and hide whether there are associated colors.
class Grid {
public:

    Grid(bool has_colors = false);

    void clear_row(size_t row);
    void set_at(size_t row, size_t col, bool filled, Color color);

    bool at(size_t row, size_t col) const;
    size_t count() const;

    void output_color_grid_to_stream(std::ostream& os) const;


private:

    BitGrid bit_grid;

    // TODO: make this into a unique pointer again and restore this classes copy/move semantics for efficiency.
    std::optional<ColorGrid> color_grid;

};


#endif