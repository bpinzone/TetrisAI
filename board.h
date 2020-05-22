#ifndef BOARD_H
#define BOARD_H

#include <bitset>
#include <vector>
#include <utility>
#include <optional>
#include <iostream>
#include <array>

struct Placement {
    int rotation;
    int column;
};

struct CH_maps {
    std::vector<int16_t> contour;
    std::vector<int16_t> height;
};

struct Block {

    // [rotation] = maps for that rotation
    std::string name;
    std::vector<CH_maps> maps;

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
    Block(const std::string& _name, const std::vector<CH_maps>& _maps)
        : name{_name}, maps{_maps} {
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

    constexpr static int c_possible_rotation_configs = 4;
    constexpr static int c_worst_case_num_placements =
        c_possible_rotation_configs * State::c_cols;

    static State worst_state;

    using Board_t = std::bitset<c_size>;

    bool place_block(const Block& b, Placement p);
    int get_num_holes() const;

    const Block* hold(const Block& b);
    const Block* get_hold() const;

    size_t populate_placements(
        const Block& b,
        std::array<Placement, c_worst_case_num_placements>::iterator dest
    ) const;

    // Returns true iff this has strictly higher utility than other.
    bool has_higher_utility_than(const State& other) const;

    Board_t::reference at(size_t row, size_t col);
    bool at(size_t row, size_t col) const;

    friend std::ostream& operator<<(std::ostream& os, const State& s);

    void print_diff_against(const State& new_other) const;

    int num_rows_cleared_on_last_place = 0;
    int tetris_count = 0;
    bool is_worst_board = false;

    void assert_cache_correct() const;

private:

    void clear_row(int row);
    int16_t get_height_map_reduction(int deleted_row, int query_col) const;
    bool is_row_full(int row) const;
    bool contour_matches(const Block& b, Placement p) const;

    // Given a block and placement, drop the block:
    // return the row idx of the left-bottom most cell of the block.
    int get_row_after_drop(const Block& b, Placement p) const;

    int get_num_trenches() const;

    std::pair<int16_t, int16_t> get_min_max_height() const;

    // Requires: get_num_trenches() just returned 1, and the board has not been modified.
    bool is_rest_board_4_above_single_trench() const;

    int16_t get_height_of_second_lowest() const;

    // Fundamental
    Board_t board;
    const Block* current_hold = nullptr;

    // Cache
    std::array<int16_t, c_cols> height_map = {0};


    int num_filled = 0;

    mutable int trench_col = 0;

};

#endif
