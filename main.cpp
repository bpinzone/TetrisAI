#include <iostream>
#include <random>
#include <functional>
#include <queue>
#include <cassert>
#include <deque>
#include <iterator>
#include <algorithm>
#include <utility>

#include "board.h"

using namespace std;

struct Decision {
    // Empty optional -> hold
    optional<Placement> placement;
    int utility;
};

Block generate_block();
optional<Decision> get_best_decision(
        const State& state, Block presented,
        deque<Block>::const_iterator next_block_it,
        const deque<Block>::const_iterator end_block_it,
        bool just_swapped);

int main() {

    State state;
    Block next_to_present = generate_block();
    deque<Block> queue;
    generate_n(back_inserter(queue), 6, &generate_block);

    for(int i = 0; i < 15; ++i){
    // while(true){
        Decision next_decision = *get_best_decision(
            state, next_to_present,
            queue.begin(), queue.begin() + 3, false
        );

        State new_state{state};
        // HOLD
        if(!next_decision.placement){
            optional<Block> old_hold = new_state.hold(next_to_present);
            if(!old_hold){
                next_to_present = queue.front();
                queue.pop_front();
                queue.push_back(generate_block());
            }
            else{
                next_to_present = *old_hold;
            }
        }
        // PLACE
        else{
            state.place_block(next_to_present, *next_decision.placement);
            next_to_present = queue.front();
            queue.pop_front();
            queue.push_back(generate_block());
        }
        cout << state << endl;
        cout << new_state << endl;
        swap(state, new_state);
    }

    return 0;
}

void pick_better_decision(
        optional<Decision>& best_decision, optional<Decision> decision,
        optional<Placement> placement,
        int new_state_utility){

    if(!decision){
        decision = {{}, new_state_utility};
    }
    decision->placement = placement;

    if(!best_decision || decision->utility > best_decision->utility){
        best_decision = decision;
    }
}

// Given the current state, returns, what is the best utility I could get?
// Action represents the next action to take if you were in the state you passed in as an argument.
// empty optional -> we will not recurse anymore. Compute utilty of the state argument you passed me yourself.
optional<Decision> get_best_decision(
        const State& state,
        Block presented,
        deque<Block>::const_iterator next_block_it,
        const deque<Block>::const_iterator end_block_it,
        bool just_swapped){

    if(next_block_it == end_block_it){
        return {};
    }

    // Try all the ways to place it.
    // Empty optional -> we haven't considered any decisions yet.
    optional<Decision> best_decision;
    Placements placements = state.get_placements(presented);
    for(const auto& placement : placements.hole){

        State new_state{state};
        new_state.place_block(presented, placement);

        optional<Decision> decision = get_best_decision(
            new_state,
            *next_block_it,
            next_block_it + 1,
            end_block_it,
            false
        );

        pick_better_decision(best_decision, decision, placement, new_state.get_utility());
    }

    if(just_swapped){
        return best_decision;
    }

    // Try holding
    State hold_state{state};
    optional<Block> was_held = hold_state.hold(presented);

    optional<Decision> decision;
    decision = get_best_decision(
        hold_state,
        was_held ? *was_held : *next_block_it,
        was_held ? next_block_it: next_block_it + 1,
        end_block_it,
        true
    );

    pick_better_decision(best_decision, decision, {}, hold_state.get_utility());

    return best_decision;
}

Block generate_block(){

    static const Block block_types[6] = {
        Block::Cyan, Block::Blue, Block::Orange,
        Block::Green, Block::Red, Block::Yellow,
    };

    // TODO: Give it a seed, or get the same game every time.
    static auto block_idx_generator = bind(
        uniform_int_distribution<int>{0, 5}, default_random_engine{}
    );
    return block_types[block_idx_generator()];

}
