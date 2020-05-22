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

class State {

public:

    // STATIC MEMBERS
    static constexpr size_t c_cols = 10;
    static constexpr size_t c_rows = 20;
    static constexpr size_t c_size = c_cols * c_rows;

    friend std::ostream& operator<<(std::ostream& os, const State& s);

    // FUNCTIONS
    // Modifying
    // Given a placement decision and block, completely modify the state.
    bool place_block(const Block& b, Placement p);
    const Block* swap_block(const Block& b);
    void become_worst_board();

    // Non-modifying

    // Returns true iff this has strictly higher utility than other.
    bool has_higher_utility_than(const State& other) const;
    int get_num_holes() const;
    bool can_swap_block(const Block& b) const;
    void print_diff_against(const State& new_other) const;

    // todo: remove
    // int get_num_tetrises() const;
    // int get_num_non_tetrises() const;

    double get_tetris_percent() const;

    static const State& get_worst_state();

private:

    using Board_t = std::bitset<c_size>;

    // FUNCTIONS
    // Modifying

    // TODO: decide on this things job.
    void clear_row(int row);

    // (0, 0) is bottom left;  (1, 0) is 2nd row, 1st column;  (0, 1) is 1st row, 2nd column.
    Board_t::reference at(size_t row, size_t col);

    // Non-modifying
    // (0, 0) is bottom left;  (1, 0) is 2nd row, 1st column;  (0, 1) is 1st row, 2nd column.
    bool at(size_t row, size_t col) const;
    bool is_row_full(int row) const;

    // Given a block and placement, drop the block:
    // return the row idx of the left-bottom most cell of the block.
    int get_row_after_drop(const Block& b, Placement p) const;
    bool contour_matches(const Block& b, Placement p) const;

    std::pair<int16_t, int16_t> get_min_max_height() const;
    int16_t get_height_of_second_lowest() const;
    int16_t get_height_map_reduction(int deleted_row, int query_col) const;

    // modifying??? Seriously consider refactoring
    int get_num_trenches() const;
    // Requires: get_num_trenches() just returned 1, and the board has not been modified.
    bool is_rest_board_4_above_single_trench() const;

    void assert_cache_correct() const;

    // MEMBERS

    static State worst_state;

    // Fundamental
    Board_t board;
    const Block* current_hold = nullptr;
    bool just_swapped = true;

    // Cache
    std::array<int16_t, c_cols> height_map = {0};
    int num_cells_filled = 0;
    // If there are 0 holes, num_cells_filled will be equal to this.
    int perfect_num_cells_filled = 0;

    // ?????
    // todo: mutable?
    mutable int some_trench_col = 0;

    // Stats
    // From first empty board, to this state, how many total tetrises occurred.
    int num_tetrises = 0;
    int num_non_tetrises = 0;
    bool is_worst_board = false;

};

#endif
