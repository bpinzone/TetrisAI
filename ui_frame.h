#ifndef UI_FRAME_H
#define UI_FRAME_H

#include <iosfwd>

#include "grid.h"
#include "state.h" // only for queue_t. TODO: decouple this.
#include "block.h"

// All the data that needs to be sent to python.
// Does not manage the lifetime of any data.
class UI_frame {
public:

    using Tetris_queue_t = State::Tetris_queue_t;

    UI_frame(
        const Grid *main_board_in,
        const Grid *best_envisioned_board_in,
        const Grid *worst_envisioned_board_in,
        const Tetris_queue_t *queue_in,
        const Block *hold_in,
        const Block *presented_in);

    void output_to_stream(std::ostream& os) const;

private:

    const Grid *main_board;
    const Grid *best_envisioned_board;
    const Grid *worst_envisioned_board;
    const State::Tetris_queue_t *queue;
    const Block *hold;
    const Block *presented;
};

#endif