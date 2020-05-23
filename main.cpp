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

const int max_holes_allowed_generated_at_once = 1;

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
            bool game_over = !new_state.place_block(*next_to_present, *next_decision.placement);
            if(game_over){
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
            State new_state{state};
            bool game_over = !new_state.place_block(presented, placement);

            State best_reachable_state_from_new_state;

            if(game_over || new_state.get_num_holes() - state.get_num_holes() > max_holes_allowed_generated_at_once){
                best_reachable_state_from_new_state = State::get_worst_state();
            }
            else if(next_block_it == end_block_it) {
                best_reachable_state_from_new_state = new_state;
            }
            else{
                best_reachable_state_from_new_state = get_best_decision(
                        new_state,
                        **next_block_it,
                        next_block_it + 1,
                        end_block_it
                ).best_foreseeable_state_from_placement;
            }

            if(!best_decision ||
                    best_reachable_state_from_new_state.has_higher_utility_than(best_decision->best_foreseeable_state_from_placement)){
                best_decision = {placement, move(best_reachable_state_from_new_state)};
            }
        }
    }

    assert(best_decision);
    if(!state.can_swap_block(presented)){
        return *best_decision;
    }

    State hold_state{state};
    const Block* was_held = hold_state.swap_block(presented);

    State best_reachable_state_from_hold_state;
    if(next_block_it == end_block_it){
        best_reachable_state_from_hold_state = hold_state;
    }
    else {
        best_reachable_state_from_hold_state = get_best_decision(
            hold_state,
            was_held ? *was_held : **next_block_it,
            was_held ? next_block_it: next_block_it + 1,
            end_block_it
        ).best_foreseeable_state_from_placement;
    }

    assert(best_decision);
    if(best_reachable_state_from_hold_state.has_higher_utility_than(best_decision->best_foreseeable_state_from_placement)){
        best_decision = {{}, move(best_reachable_state_from_hold_state)};
    }

    assert(best_decision);
    return *best_decision;
}

