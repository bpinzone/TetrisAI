#ifndef BOARD_H
#define BOARD_H

#include <bitset>
#include <vector>
#include <utility>
#include <optional>

struct Placement {
    int rotation;
    int position;
};

struct Placements {
    std::vector<Placement> non_hole;
    std::vector<Placement> hole;
};

// Cyan is long boi.
enum class Block {
    Cyan, Blue, Orange, Green, Red, Yellow
};


// (0, 0) is bottom left.
// (1, 0) is 2nd row, 1st column.
// (0, 1) is 1st row, 2nd column.
class State {

public:

    void place_block(Block b, Placement p);

    std::optional<Block> set_hold(Block b);
    std::optional<Block> get_hold() const;

    Placements get_placements(Block b) const;

    double get_utility() const;

private:

    constexpr static size_t c_cols = 10;
    constexpr static size_t c_rows = 20;
    constexpr static size_t c_size = c_cols * c_rows;

    void remove_row(int row);

    // Fundamental
    std::bitset<c_size> board;
    std::optional<Block> hold;

    // Cache
    int16_t height_map[c_cols];


};

#endif