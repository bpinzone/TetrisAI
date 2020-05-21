#ifndef BOARD_H
#define BOARD_H

#include <bitset>
#include <vector>
#include <utility>
#include <optional>
#include <iostream>
#include <array>
#include <algorithm>
#include <iterator>

struct Placement {
    int rotation;
    int column;
};


struct CH_maps {
private:
    inline static const size_t c_max_lateral_length = 4;
public:
    int16_t contour[c_max_lateral_length];
    int16_t height[c_max_lateral_length];
    // TODO: Initialize
    int size;
};

struct Block {

private:
    inline static constexpr size_t c_max_num_rotations = 4;

public:

    using Maps_std_arr_t = std::array<CH_maps, c_max_num_rotations>;

    // [rotation] = maps for that rotation
    std::string name;

    // TODO: Initialize.
    int maps_size;
    CH_maps maps[c_max_num_rotations];
    // std::vector<CH_maps> maps;

    static const Block Cyan;
    static const Block Blue;
    static const Block Orange;
    static const Block Green;
    static const Block Red;
    static const Block Yellow;
    static const Block Purple;

    Block& operator=(const Block& other) = delete;
    Block& operator=(Block&& other) = delete;

    Block(const Block& other) = delete;
    Block(Block&& other) = delete;

private:
    Block(
        const std::string& _name, int _maps_size,
        Maps_std_arr_t _maps)
        : name{_name}, maps_size{_maps_size} {

        std::copy(std::begin(_maps), std::end(_maps), std::begin(maps));
    }
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

    using Placements_t = std::vector<Placement>;

    constexpr static size_t c_cols = 10;
    constexpr static size_t c_rows = 20;
    constexpr static size_t c_size = c_cols * c_rows;

    using Board_t = std::bitset<c_size>;

    bool place_block(const Block& b, Placement p);
    int get_num_holes() const;

    const Block* hold(const Block& b);
    const Block* get_hold() const;

    Placements_t get_placements(const Block& b) const;

    double get_utility() const;

    Board_t::reference at(size_t row, size_t col);
    bool at(size_t row, size_t col) const;

    friend std::ostream& operator<<(std::ostream& os, const State& s);

    void print_diff_against(const State& new_other) const;

private:

    void clear_row(int row);
    bool is_row_full(int row) const;
    bool contour_matches(const Block& b, Placement p) const;

    // Given a block and placement, drop the block:
    // return the row idx of the left-bottom most cell of the block.
    int get_row_after_drop(const Block& b, Placement p) const;

    // Fundamental
    Board_t board;
    const Block* current_hold = nullptr;

    // Cache
    std::array<int16_t, c_cols> height_map = {0};
};

#endif
