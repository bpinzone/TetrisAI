#ifndef BOARD_H
#define BOARD_H

#include <bitset>
#include <vector>
#include <utility>
#include <optional>
#include <iostream>

struct Placement {
    int rotation;
    int column;
};

struct Placements {
    // std::vector<Placement> non_hole;
    std::vector<Placement> hole;
};


struct CH_maps {
    std::vector<int16_t> contour;
    std::vector<int16_t> height;
};

struct Block {

    // [rotation] = maps for that rotation
    std::vector<CH_maps> maps;

    static const Block Cyan;
    static const Block Blue;
    static const Block Orange;
    static const Block Green;
    static const Block Red;
    static const Block Yellow;

};

// Cyan is long boi.
// enum class Block {
//     Cyan, Blue, Orange, Green, Red, Yellow
// };


// (0, 0) is bottom left.
// (1, 0) is 2nd row, 1st column.
// (0, 1) is 1st row, 2nd column.
class State {

public:

    constexpr static size_t c_cols = 10;
    constexpr static size_t c_rows = 20;
    constexpr static size_t c_size = c_cols * c_rows;

    using Board_t = std::bitset<c_size>;

    void place_block(Block b, Placement p);

    std::optional<Block> hold(Block b);
    std::optional<Block> get_hold() const;

    Placements get_placements(Block b) const;

    int get_utility() const;

    Board_t::reference at(size_t row, size_t col);
    bool at(size_t row, size_t col) const;

    friend std::ostream& operator<<(std::ostream& os, const State& s);

private:

    void remove_row(int row);
    bool is_row_full(int row) const;

    // Given a block and placement, drop the block:
    // return the row idx of the left-bottom most cell of the block.
    int get_row_after_drop(Block b, Placement p) const;

    // Fundamental
    Board_t board;
    std::optional<Block> current_hold;

    // Cache
    std::vector<int16_t> height_map = std::vector<int16_t>(c_cols, 0);
};

#endif
