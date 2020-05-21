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

#include "board.h"

using namespace std;

using Placements_t = State::Placements_t;

struct Decision {
    // Empty optional -> hold
    optional<Placement> placement;
    double utility;
};

static const int placement_lookahead_depth = 4;
static const int placements_to_perform = 300;

void play();
void test();

const Block* generate_block();
optional<Decision> get_best_decision(
        const State& state,
        const Block& presented,
        const deque<const Block*>::const_iterator& next_block_it,
        const deque<const Block*>::const_iterator& end_block_it,
        bool just_swapped,
        int old_num_holes);

int main() {

    // cout << sizeof(State) << endl;
    // cout << sizeof(State::Board_t) << endl;
    // cout << sizeof(vector<int16_t>) << endl;
    // return 0;

    play();
    // test();

    return 0;
}

void test(){

    State state;
    state.place_block(Block::Cyan, {0, 0});
    state.place_block(Block::Cyan, {0, 0});
    state.place_block(Block::Cyan, {1, 9});
    cout << state << endl;

}

void play(){

    State state;
    const Block* next_to_present = generate_block();
    deque<const Block*> queue;
    generate_n(back_inserter(queue), 6, &generate_block);

    int turn = 0;
    while(turn < placements_to_perform){
    // while(true){
        cout << "Turn: " << turn << endl;
        cout << "Presented with: " << next_to_present->name << endl;
        Decision next_decision = *get_best_decision(
            state, *next_to_present,
            queue.begin(), queue.begin() + placement_lookahead_depth, false,
            state.get_num_holes()
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
        double new_state_utility){

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
        const Block& presented,
        const deque<const Block*>::const_iterator& next_block_it,
        const deque<const Block*>::const_iterator& end_block_it,
        bool just_swapped,
        int old_num_holes){

    if(next_block_it == end_block_it || state.get_num_holes() - old_num_holes >= 2){
        return {};
    }

    // Try all the ways to place it.
    // Empty optional -> we haven't considered any decisions yet.
    optional<Decision> best_decision;
    Placements_t placements = state.get_placements(presented);
    for(const auto& placement : placements){

        State new_state{state};
        bool success = new_state.place_block(presented, placement);
        optional<Decision> decision;
        if(success){
            decision = get_best_decision(
                new_state,
                **next_block_it,
                next_block_it + 1,
                end_block_it,
                false,
                state.get_num_holes()
            );
        }
        else{
            // We cant place this block in this orientation without going too high.
            // We lose, utility lowest
            decision = {placement, numeric_limits<double>::lowest()};
        }

        pick_better_decision(best_decision, decision, placement, new_state.get_utility());
    }

    if(just_swapped){
        return best_decision;
    }

    // Try holding
    State hold_state{state};
    const Block* was_held = hold_state.hold(presented);

    optional<Decision> decision;
    decision = get_best_decision(
        hold_state,
        was_held ? *was_held : **next_block_it,
        was_held ? next_block_it: next_block_it + 1,
        end_block_it,
        true,
        state.get_num_holes()
    );

    pick_better_decision(best_decision, decision, {}, hold_state.get_utility());

    return best_decision;
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
