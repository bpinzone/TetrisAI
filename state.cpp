#include "state.h"
#include "block.h"
#include "global_stats.h"

#include <iostream>

using std::optional;
using std::ostream;

// === STATE ===

State::State(
        const Board& _board,
        const Block* _presented_block,
        const Tetris_queue_t::const_iterator& _next_queue_it,
        bool _is_leaf,
        const Tetris_queue_t::const_iterator& _end_queue_it,
        int _placement_limit,
        std::optional<Placement> _placement_taken_from_root)

    : board{_board}, presented_block{_presented_block}, next_queue_it{_next_queue_it},
    is_leaf{_is_leaf}, end_queue_it{_end_queue_it}, placement_limit{_placement_limit},
    placement_taken_from_root{_placement_taken_from_root}, pg{_presented_block} {
}

State State::generate_root_state(
        const Board& board,
        const Block& presented,
        const Tetris_queue_t& queue,
        const int num_placements_to_look_ahead){

    const int placement_limit = board.get_num_blocks_placed() + num_placements_to_look_ahead;

    return {
        board,
        &presented,
        queue.cbegin(),
        false,
        queue.cend(),
        placement_limit,
        optional<Placement>{}
    };

}

optional<State> State::generate_next_child() {

    for(optional<Placement> placement = pg(); placement; placement = pg()){
        optional<State> child = generate_child_from_placement(*placement);
        if(child){
            // if(child->get_is_leaf()){
            //     incr_num_leaves();
            // }
            return child;
        }
    }
    return {};
}


// Given the current state, attempt to generate a new state with a placement.
// Empty optional means:
// In the case of putting a block down the child was not promising.
// In the case of a hold, the hold could not be done.
optional<State> State::generate_child_from_placement(Placement placement){

    Board child_board{board};

    if(!placement.get_is_hold()){

        bool is_child_promising = child_board.place_block(*presented_block, placement);

        if(is_child_promising){

            bool is_leaf =
                (child_board.get_num_blocks_placed() == placement_limit)
                || (next_queue_it == end_queue_it);

            return State{
                child_board,
                // If next_queue_it is the end iterator, this is a terminal node, and presented will never be read.
                // Choice of Cyan is arbitrary.
                is_leaf ? &Block::Cyan : *next_queue_it,
                next_queue_it + 1,
                is_leaf,
                end_queue_it,
                placement_limit,
                placement_taken_from_root ? placement_taken_from_root : optional<Placement>{placement}
            };
        }
    }
    else if(board.can_swap_block(*presented_block)){

        bool is_leaf = next_queue_it == end_queue_it;
        const Block* was_held = child_board.swap_block(*presented_block);
        return State{
            child_board,
            is_leaf ? &Block::Cyan : (was_held ? was_held : *next_queue_it),
            was_held ? next_queue_it : next_queue_it + 1,
            is_leaf,
            end_queue_it,
            placement_limit,
            placement_taken_from_root ? placement_taken_from_root : optional<Placement>{placement}
        };
    }
    return {};
}

// ===================   Placement Generator     ==============================

// Returns all possible placements, and then an empty optional when none remain.
State::Placement_generator::Placement_generator(const Block* _presented)
    : presented{_presented}, rot_x{0}, col{0}, exhausted{false} {
}

// WE WANT COROUTINES
// Emulates:
// for(int rot_x = 0; rot_x < presented.maps.size(); ++rot_x){
//     const int max_valid_col = Board::c_cols - presented.maps[rot_x].contour.size();
//     for(int col = 0; col <= max_valid_col; ++col){
optional<Placement> State::Placement_generator::operator()(){

    if(exhausted){
        return {};
    }

    for(; rot_x < presented->maps.size(); ++rot_x){
        const int max_valid_col = Board::c_cols - presented->maps[rot_x].contour.size();
        // Only time rot_x is incremented is when this is false.
        while(col <= max_valid_col){
            // Increment col every time. Do so before control leaves.
            return {{rot_x, col++, false}};
        }
        col = 0;
    }

    exhausted = true;
    // Returning the hold placement. 0's are never read.
    return {{0, 0, true}};
}

ostream& operator<<(ostream& os, const State& state){

    os << state.board << "\n";

    return os;
}