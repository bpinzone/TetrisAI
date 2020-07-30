#ifndef BOARD_H
#define BOARD_H

#include <bitset>
#include <utility>
#include <optional>
#include <array>

#include <iosfwd>

struct Block;
struct Placement;

struct Board_lifetime_stats {
    int num_blocks_placed = 0;
    int num_placements_that_cleared_rows = 0;
    int num_tetrises = 0;
    int num_non_tetrises = 0;
    int num_all_clears = 0;
    double max_height_exp_moving_average = 0;
};

class Board {

public:

    Board(){
    }

    Board(std::istream& is);

    // STATIC MEMBERS
    static constexpr size_t c_cols = 10;
    static constexpr size_t c_rows = 20;
    static constexpr size_t c_size = c_cols * c_rows;

    friend std::ostream& operator<<(std::ostream& os, const Board& s);

    // FUNCTIONS
    // Modifying
    // Given a placement decision and block, completely modify the state.
    bool place_block(const Block& b, Placement p);
    const Block* swap_block(const Block& b);
    void set_lifetime_stats(const Board_lifetime_stats& new_board_lifetime_stats);

    // Non-modifying

    // Returns true iff this has strictly higher utility than other.
    bool has_greater_utility_than(const Board& other) const;
    int get_num_holes() const;
    bool can_swap_block(const Block& b) const;
    bool is_holding_some_block() const;

    int get_num_blocks_placed() const;
    double get_tetris_percent() const;
    bool has_more_cleared_rows_than(const Board& other) const;
    bool is_clear() const;

    Board_lifetime_stats get_lifetime_stats() const;

    static const Board& get_worst_board();

private:

    using Grid_t = std::bitset<c_size>;

    // FUNCTIONS
    // Modifying

    void clear_row(int row);

    // (0, 0) is bottom left;  (1, 0) is 2nd row, 1st column;  (0, 1) is 1st row, 2nd column.
    Grid_t::reference at(size_t row, size_t col);

    // Non-modifying
    // (0, 0) is bottom left;  (1, 0) is 2nd row, 1st column;  (0, 1) is 1st row, 2nd column.
    bool at(size_t row, size_t col) const;
    bool is_row_full(int row) const;
    int compute_height(size_t col_x) const;

    // Given a block and placement, drop the block:
    // return the row idx of the left-bottom most cell of the block.
    int get_row_after_drop(const Block& b, Placement p) const;

    int get_height_map_reduction(int deleted_row, int query_col) const;

    // Fundamental and Primary cache data must be up to date before calling update second/life cache.
    void update_secondary_cache();
    void update_lifetime_cache(int num_rows_cleared_just_now);

    // MEMBERS
    static Board worst_board;

    // === Fundamental ===
    Grid_t board;

    const Block* current_hold = nullptr;
    bool just_swapped = true;

    // === Primary Cache. Should be updated in place_block() and clear_row() ===
    std::array<int, c_cols> height_map = {0};
    int num_cells_filled = 0;
    // If there are 0 holes, num_cells_filled will be equal to this.
    int perfect_num_cells_filled = 0;

    // === Secondary Cache. Relies on info in Primary Cache being up to date to compute these.
    /*
    // Cached second. Should be updated in update_secondary_cache().
    Update cache is responsible for the following.
    None of these are ever read by place_block() or clear_row()
    */
    int num_trenches = 0;
    bool at_least_one_side_clear = true;
    int lowest_height = 0;
    int second_lowest_height = 0;
    int highest_height = 0;
    int sum_of_squared_heights = 0;
    // Assuming no holes, is true iff a cyan could be placed for a tetris right now.
    bool is_tetrisable = false;

    // === Lifetime Cache ===
    // Stats that you could not infer just from viewing the board.
    Board_lifetime_stats lifetime_stats;

    // === Misc ===
    bool is_worst_board = false;

};


#endif
