#include "ui_frame.h"

#include <iostream>

UI_frame::UI_frame(const Grid *main_board_in)
    : main_board(main_board_in)
{}

void UI_frame::output_to_stream(std::ostream& os) const {
    os << "main_board:\n";
    main_board->output_color_grid_to_stream(os);
    os.flush();
}