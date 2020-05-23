#include <iostream>
#include <random>
#include <functional>
#include <queue>
#include <cassert>
#include <deque>
#include <iterator>
#include <algorithm>
#include <utility>
#include <limits>
#include <array>
#include <cstdlib>

#include "board.h"

using namespace std;

using Seed_t = unsigned int;

struct Decision {
    // Empty optional -> hold
    optional<Placement> placement;
    /* Extract utility from this */
    State best_foreseeable_state_from_placement;
};

void play(Seed_t seed, int queue_consume_lookahead_depth, int placements_to_perform);

class Block_generator {

public:

    Block_generator(Seed_t seed)
        : generator{seed} {
    }

    const Block* operator()() {
        return block_ptrs[distribution(generator)];
    }

private:

    inline static const Block* block_ptrs[] = {
        &Block::Cyan, &Block::Blue, &Block::Orange,
        &Block::Green, &Block::Red, &Block::Yellow, &Block::Purple
    };

    inline static uniform_int_distribution<int> distribution{
        0, (sizeof(block_ptrs) / sizeof(block_ptrs[0])) - 1
    };

    default_random_engine generator;
};

Decision get_best_decision(
        const State& state,
        const Block& presented,
        const deque<const Block*>::const_iterator& next_block_it,
        const deque<const Block*>::const_iterator& end_block_it);

// <prog> <seed> <lookahead_depth> <placements_to_perform>
int main(int argc, char* argv[]) {
    if(argc != 4){
        cout << "Incorrect usage: Requires: <seed> <depth> <placement count>" << endl;
        return 0;
    }
    play(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]));
    return 0;
}

void play(Seed_t seed, int queue_consume_lookahead_depth, int placements_to_perform){

    Block_generator block_generator(seed);

    State state;
    const Block* next_to_present = block_generator();
    deque<const Block*> queue;
    generate_n(back_inserter(queue), 6, block_generator);

    int turn = 0;
    while(turn < placements_to_perform){

        // Status
        cout << "Turn: " << turn << "\n";
        cout << "Tetris percent:" << state.get_tetris_percent() << " %" << endl;
        cout << "Presented with: " << next_to_present->name << endl;

        Decision next_decision = get_best_decision(
            state, *next_to_present,
            queue.begin(), queue.begin() + queue_consume_lookahead_depth
        );

        State new_state{state};
        // HOLD
        if(!next_decision.placement){
            const Block* old_hold = new_state.swap_block(*next_to_present);
            if(!old_hold){
                next_to_present = queue.front();
                queue.pop_front();
                queue.push_back(block_generator());
            }
            else{
                next_to_present = old_hold;
            }
        }
        // PLACE
        else{
            bool is_promising = new_state.place_block(*next_to_present, *next_decision.placement);
            if(!is_promising){
                cout << "Game over :(" << endl;
                return;
            }
            next_to_present = queue.front();
            queue.pop_front();
            queue.push_back(block_generator());
        }
        state.print_diff_against(new_state);
        cout << endl;
        swap(state, new_state);
        ++turn;
    }

}

// TODO: Update comment
// Returns, given the state, "state", the state s' with the best utility which can be reached from "state"
Decision get_best_decision(
        const State& state,
        const Block& presented,
        const deque<const Block*>::const_iterator& next_block_it,
        const deque<const Block*>::const_iterator& end_block_it){

    // Empty optional -> we haven't considered any decisions yet.
    optional<Decision> best_decision;

    // Try all placement options.
    for(int rot_x = 0; rot_x < presented.maps.size(); ++rot_x){
        const int max_valid_col = State::c_cols - presented.maps[rot_x].contour.size();
        for(int col = 0; col <= max_valid_col; ++col){

            const Placement& placement = {rot_x, col};
            State best_reachable_state_with_placement{state};
            bool is_promising = best_reachable_state_with_placement.place_block(presented, placement);

            if(!is_promising){
                best_reachable_state_with_placement = State::get_worst_state();
            }
            else if(next_block_it != end_block_it) {
                best_reachable_state_with_placement = get_best_decision(
                        best_reachable_state_with_placement,
                        **next_block_it,
                        next_block_it + 1,
                        end_block_it
                ).best_foreseeable_state_from_placement;
            }

            if(!best_decision ||
                    best_reachable_state_with_placement.has_higher_utility_than(best_decision->best_foreseeable_state_from_placement)){
                best_decision = {placement, move(best_reachable_state_with_placement)};
            }
        }
    }

    assert(best_decision);
    if(!state.can_swap_block(presented)){
        return *best_decision;
    }

    State best_state_reachable_with_hold{state};
    const Block* was_held = best_state_reachable_with_hold.swap_block(presented);

    if(next_block_it != end_block_it){
        best_state_reachable_with_hold = get_best_decision(
            best_state_reachable_with_hold,
            was_held ? *was_held : **next_block_it,
            was_held ? next_block_it: next_block_it + 1,
            end_block_it
        ).best_foreseeable_state_from_placement;
    }

    assert(best_decision);
    if(best_state_reachable_with_hold.has_higher_utility_than(best_decision->best_foreseeable_state_from_placement)){
        best_decision = {{}, move(best_state_reachable_with_hold)};
    }

    assert(best_decision);
    return *best_decision;
}

