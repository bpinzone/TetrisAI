#ifndef COLOR_GRID_H
#define COLOR_GRID_H

#include <array>

#include "board_size.h"
#include "block.h"

struct ColorCellShade {
    bool is_filled;
    bool is_ghost;
    bool is_about_to_be_cleared;

    ColorCellShade(){
        is_filled = false;
        is_ghost = false;
        is_about_to_be_cleared = false;
    }

    void output_to_stream(std::ostream& os) const;
};

struct ColorCell {
    Color color;
    ColorCellShade state; // todo: rename to shade.
    void output_to_stream(std::ostream& os) const;
};


class ColorGrid {
public:

    void set_at(size_t row, size_t col, ColorCell cell);
    void clear_row(size_t deleted_row);

    // rely on bit grid to tell you if the game is over.
    void add_junk(int pos, int count);

    void output_to_stream(std::ostream& os) const;

private:

    using InternalGrid_t = std::array<
        std::array<ColorCell, BoardSize::c_cols>,
        BoardSize::c_rows>;

    InternalGrid_t board;


};




#endif
