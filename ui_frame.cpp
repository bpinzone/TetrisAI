#include "ui_frame.h"

#include <iostream>

UI_frame::UI_frame(
    const Grid *main_board_in,
    const Grid *best_envisioned_board_in,
    const Grid *worst_envisioned_board_in,
    const Tetris_queue_t *queue_in,
    const Block *hold_in,
    const Block *presented_in
)
    : main_board(main_board_in),
    best_envisioned_board(best_envisioned_board_in),
    worst_envisioned_board(worst_envisioned_board_in),
    queue(queue_in),
    hold(hold_in),
    presented(presented_in)
{}

void UI_frame::output_to_stream(std::ostream& os) const {

    os << "ui_frame:\n";

    os << "queue:";
    os << "\n";
    for(const auto& block : *queue) {
        os << block->name;
    }
    os << "\n";

    os << "hold:";
    os << "\n";
    if(hold) {
        os << hold->name;
    }
    else {
        os << "n";
    }
    os << "\n";

    os << "presented:";
    os << "\n";
    os << presented->name;
    os << "\n";


    os << "main_board:\n";
    main_board->output_color_grid_to_stream(os);

    os << "best_envisioned_board:\n";
    best_envisioned_board->output_color_grid_to_stream(os);

    os << "worst_envisioned_board:\n";
    worst_envisioned_board->output_color_grid_to_stream(os);
    os.flush();
    
    
}