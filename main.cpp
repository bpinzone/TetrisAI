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

#include "board.h"

using namespace std;

using Placements_t = State::Placements_t;

struct Decision {
    // Empty optional -> hold
    optional<Placement> placement;
    /* Extract utility from this */
    State best_foreseeable_state_from_placement;
};

static const int queue_consume_lookahead_depth = 5;
static const int placements_to_perform = 15;

void play();

const Block* generate_block();
Decision get_best_decision(
        const State& state,
        const Block& presented,
        const deque<const Block*>::const_iterator& next_block_it,
        const deque<const Block*>::const_iterator& end_block_it);

int main() {
    State::worst_state.is_worst_board = true;
    play();
    return 0;
}

void play(){

    State state;
    const Block* next_to_present = generate_block();
    deque<const Block*> queue;
    generate_n(back_inserter(queue), 6, &generate_block);

    int turn = 0;
    int tetris_count = 0;
    int non_tetris_count = 0;
    while(turn < placements_to_perform){
        state.assert_cache_correct();
        cout << "Turn: " << turn << "\n";
        cout << "Tetris percent:"
             << ((static_cast<double>(tetris_count))/(tetris_count + non_tetris_count)) * 100.0
             << " %\n";
        cout << "Presented with: " << next_to_present->name << endl;
        state.tetris_count = 0;
        Decision next_decision = get_best_decision(
            state, *next_to_present,
            queue.begin(), queue.begin() + queue_consume_lookahead_depth
        );

        State new_state{state};
        // HOLD
        if(!next_decision.placement){
            const Block* old_hold = new_state.hold(*next_to_present);
            if(!old_hold){
                next_to_present = queue.front();
                queue.pop_front();
                queue.push_back(generate_block());
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
            if(new_state.num_rows_cleared_on_last_place == 4){
                ++tetris_count;
            }
            else if (new_state.num_rows_cleared_on_last_place > 0){
                ++non_tetris_count;
            }
            next_to_present = queue.front();
            queue.pop_front();
            queue.push_back(generate_block());
        }
        state.print_diff_against(new_state);
        cout << endl;
        swap(state, new_state);
        ++turn;
    }

}

void pick_better_decision(
        optional<Decision>& best_decision, optional<Decision> decision,
        optional<Placement> placement,
        const State& new_state ){

    if(!decision){
        decision = {{}, new_state};
    }
    decision->placement = placement;

    if(
            !best_decision ||
            decision->best_foreseeable_state_from_placement.has_higher_utility_than(
                best_decision->best_foreseeable_state_from_placement)){
        best_decision = decision;
    }
}

// Given the current state, returns, what is the best utility I could get?
// Action represents the next action to take if you were in the state you passed in as an argument.
// empty optional -> we will not recurse anymore. Compute utilty of the state argument you passed me yourself.

// TODO: Update comment
// Returns, given the state, "state", the state s' with the best utility which can be reached from "state"
Decision get_best_decision(
        const State& state,
        const Block& presented,
        const deque<const Block*>::const_iterator& next_block_it,
        const deque<const Block*>::const_iterator& end_block_it){

    // Try all the ways to place it.
    array<Placement, State::c_worst_case_num_placements> placements;

    // Empty optional -> we haven't considered any decisions yet.
    optional<Decision> best_decision;
    size_t placements_size = state.populate_placements(presented, placements.begin());
    for(size_t placement_x = 0; placement_x < placements_size; ++placement_x){

        const auto& placement = placements[placement_x];
        State new_state{state};
        bool game_over = !new_state.place_block(presented, placement);

        State best_reachable_state_from_new_state;

        if(game_over || new_state.get_num_holes() - state.get_num_holes() >= 2){
            best_reachable_state_from_new_state = State::worst_state;
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

    assert(best_decision);
    if(!state.get_can_hold(presented)){
        return *best_decision;
    }

    State hold_state{state};
    const Block* was_held = hold_state.hold(presented);

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

const Block* generate_block(){

    static const Block* block_ptrs[] = {
        &Block::Cyan, &Block::Blue, &Block::Orange,
        &Block::Green, &Block::Red, &Block::Yellow, &Block::Purple
    };

    // TODO: Give it a seed, or get the same game every time.
    static auto block_idx_generator = bind(
        uniform_int_distribution<int>{
            0,
            (sizeof(block_ptrs) / sizeof(block_ptrs[0])) - 1
        },
        default_random_engine{}
    );
    return block_ptrs[block_idx_generator()];

}
