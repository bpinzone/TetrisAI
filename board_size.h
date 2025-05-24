#ifndef BOARD_SIZE_H
#define BOARD_SIZE_H

#include <cstddef>

namespace BoardSize {
    static constexpr std::size_t c_cols = 10;
    static constexpr std::size_t c_rows = 20;
    static constexpr std::size_t c_size = c_cols * c_rows;
}

#endif 