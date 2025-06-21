#ifndef BOARD_H
#define BOARD_H

#include <bitset>
#include <utility>
#include <optional>
#include <array>

#include <iosfwd>

#include "board_size.h"
#include "grid.h"
#include "board_comp.h"
#include "board_foundation.h"
#include "board_deriv.h"

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

struct Ancestor_data {
    int highest_height = 0;
    int second_lowest_height = 0;
    bool good_trench_status = true;
};

class Board {

public:

    Board() : foundation(false) { }
    Board(bool has_colors) : foundation(has_colors) { }

    Board(std::istream& is);

    friend std::ostream& operator<<(std::ostream& os, const Board& s);

    // FUNCTIONS
    // Modifying
    // Given a placement decision and block, completely modify the state.
    bool place_block(const Block& b, Placement p);
    const Block* swap_block(const Block& b);
    void set_lifetime_stats(const Board_lifetime_stats& new_board_lifetime_stats);
    void load_ancestral_data_with_current_data();

    // returns true iff the game is over.
    bool add_junk(int pos, int count);


    // Non-modifying

    // Returns true iff this has strictly higher utility than other.
    bool has_greater_utility_than(const Board& other) const;
    bool can_swap_block(const Block& b) const;
    bool is_holding_some_block() const;
    const Block * get_hold() const;

    int get_num_blocks_placed() const;
    double get_tetris_percent() const;
    bool has_more_cleared_rows_than(const Board& other) const;
    bool is_clear() const;

    Board_lifetime_stats get_lifetime_stats() const;

    const Grid *get_grid() const;

private:

    // FUNCTIONS
    // Modifying

    // Non-modifying
    // (0, 0) is bottom left;  (1, 0) is 2nd row, 1st column;  (0, 1) is 1st row, 2nd column.
    bool at(size_t row, size_t col) const;
    bool is_promising() const;
    int num_holes_above_height(int height) const;

    // Fundamental and Primary cache data must be up to date before calling update second/life cache.
    void update_secondary_cache();
    void update_lifetime_cache(int num_rows_cleared_just_now);


    Board_foundation foundation;
    Board_deriv deriv;


    // === Lifetime Cache ===
    // Stats that you could not infer just from viewing the board.
    Board_lifetime_stats lifetime_stats;

    // === Ancestral Data. Choose carefully when to manipulate this. ===
    Ancestor_data ancestor_with_smallest_max_height;

};


#endif
