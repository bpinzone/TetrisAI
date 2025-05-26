#ifndef UI_FRAME_H
#define UI_FRAME_H

#include <iosfwd>

#include "grid.h"

// All the data that needs to be sent to python.
// Does not manage the lifetime of any data.
class UI_frame {

public:

    UI_frame(const Grid *main_board_in);

    void output_to_stream(std::ostream& os) const;

private:

    const Grid *main_board;
};

#endif