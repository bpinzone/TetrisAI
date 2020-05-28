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
using Tetris_queue_t = deque<const Block*>;

void play(Seed_t seed, int queue_consume_lookahead_depth, int placements_to_perform);

optional<Placement> get_best_move(
        const State& state,
        const Block& presented,
        const Tetris_queue_t& queue,
        const int num_placements_to_look_ahead);

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


// <prog> <seed> <lookahead_depth> <placements_to_perform>
int main(int argc, char* argv[]) {
    if(argc != 4){
        cout << "Incorrect usage: Requires: <seed> <depth> <placement count>" << endl;
        return 0;
    }
    play(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]));
    return 0;
}

void play(Seed_t seed, int num_placements_to_look_ahead, int placements_to_perform){

    if(num_placements_to_look_ahead > 7){
        cout << "Jeff cannot see that far into the future. Enter 7 or less.\n";
        return;
    }
    if(num_placements_to_look_ahead < 1){
        cout << "Jeff must be able to place a block!\n";
        return;
    }

    Block_generator block_generator(seed);

    State state;
    const Block* next_to_present = block_generator();
    Tetris_queue_t queue;
    generate_n(back_inserter(queue), 6, block_generator);

    int turn = 0;
    while(turn < placements_to_perform){

        // Status
        cout << "Turn: " << turn << "\n";
        cout << "Tetris percent:" << state.get_tetris_percent() << " %" << endl;
        cout << "Presented with: " << next_to_present->name << endl;

        optional<Placement> next_placement = get_best_move(
            state, *next_to_present,
            queue, num_placements_to_look_ahead
        );

        State new_state{state};
        // HOLD
        if(!next_placement){
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
            bool is_promising = new_state.place_block(*next_to_present, *next_placement);
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


// Returns all possible placements, and then an empty optional to indicate hold.
// Caller should not generate more values after empty optional is returned.
class Placement_generator{

public:

    Placement_generator(const Block& _presented)
        : presented{_presented}, rot_x{0}, col{-1},
        max_valid_col{presented.get_max_valid_placement_col(rot_x)} {
    }

    // Emulates:
    // for(int rot_x = 0; rot_x < presented.maps.size(); ++rot_x){
    //     const int max_valid_col = State::c_cols - presented.maps[rot_x].contour.size();
    //     for(int col = 0; col <= max_valid_col; ++col){

    optional<Placement> operator()(){

        assert(!exhausted);
        ++col;
        if(col > max_valid_col){
            ++rot_x;
            if(rot_x >= presented.maps.size()){
                exhausted = true;
                return {};
            }
            max_valid_col = presented.get_max_valid_placement_col(rot_x);
            col = 0;
            return {{rot_x, col}};
        }
        return {{rot_x, col}};
    }

private:

    const Block& presented;
    int rot_x;
    int col;
    int max_valid_col;
    bool exhausted = false;
};

// TODO: rename this to state. And old "State" to "Board"
struct Decision_point {
    State state;
    const Block& presented_block;
    Tetris_queue_t::const_iterator next_queue_it;
    bool is_leaf = false;

    Decision_point(
            const State& _state,
            const Block& _presented_block,
            const Tetris_queue_t::const_iterator& _next_queue_it,
            bool _is_leaf)
        : state{_state}, presented_block{_presented_block}, next_queue_it{_next_queue_it}, is_leaf{_is_leaf} {
    }



};


// REQUIRES: if placement is null, the parent state is able to perform a hold.
// Do not call this if parent_decision is terminal
// Returns empty optional if the child isn't promising.
optional<Decision_point> generate_child_decision_point(
        const Decision_point& parent_decision,
        optional<Placement> placement,
        Tetris_queue_t::const_iterator queue_end_it,
        int placement_limit){

    State child{parent_decision.state};

    if(placement){

        bool is_child_promising =
            child.place_block(parent_decision.presented_block, *placement);

        bool is_leaf = child.get_num_blocks_placed() == placement_limit || parent_decision.next_queue_it == queue_end_it;

        return is_child_promising ?
            optional<Decision_point>{{
                child,
                // If next_queue_it is the end iterator, this is a terminal node, and presented will never be read.
                // Choice of Cyan is arbitrary.
                is_leaf ? Block::Cyan : **parent_decision.next_queue_it,
                parent_decision.next_queue_it + 1,
                is_leaf
            }}
            : optional<Decision_point>{};
    }
    else {

        assert(parent_decision.state.can_swap_block(parent_decision.presented_block));
        bool is_leaf = parent_decision.next_queue_it == queue_end_it;
        const Block* was_held = child.swap_block(parent_decision.presented_block);
        return {{
                child,
                is_leaf ? Block::Cyan : (was_held ? *was_held : **parent_decision.next_queue_it),
                was_held ? parent_decision.next_queue_it : parent_decision.next_queue_it + 1,
                is_leaf
        }};
    }

}

State get_best_reachable_state(
        const Decision_point&& root_decision_point,
        Tetris_queue_t::const_iterator queue_end_it,
        int placement_limit){

    State best_foreseeable_state = State::get_worst_state();

    vector<Decision_point> state_stack;
    state_stack.push_back(move(root_decision_point));

    while(!state_stack.empty()){

        Decision_point considered_state = state_stack.back();
        state_stack.pop_back();

        if(considered_state.is_leaf){
            if(considered_state.state.has_greater_utility_than(best_foreseeable_state)){
                best_foreseeable_state = considered_state.state;
            }
        }
        else{

            Placement_generator pg{considered_state.presented_block};
            // TODO: reduce duplication with while loop in get_best_move.
            while(true){

                optional<Placement> placement = pg();
                if(placement || considered_state.state.can_swap_block(considered_state.presented_block)){

                    optional<Decision_point> child_decision_point = generate_child_decision_point(
                        considered_state,
                        placement,
                        queue_end_it,
                        placement_limit);

                    if(!child_decision_point){
                        continue;
                    }

                    state_stack.push_back(*child_decision_point);
                }

                if(!placement){
                    break;
                }
            }

        } // non-terminal else
    } // while

    return best_foreseeable_state;

}

optional<Placement> get_best_move(
        const State& state,
        const Block& presented,
        const Tetris_queue_t& queue,
        const int num_placements_to_look_ahead){

    State best_foreseeable_state = State::get_worst_state();
    // Empty optional -> hold
    // Init to some move, so that we don't return empty optional when nothing is good.
    optional<Placement> best_move = {{0, 0}};

    Decision_point first_decision_point {
        state, presented, queue.cbegin(), false
    };

    // First placement generator
    Placement_generator fpg{presented};
    const int placement_limit = state.get_num_blocks_placed() + num_placements_to_look_ahead;
    while(true){

        optional<Placement> placement = fpg();

        if(placement || first_decision_point.state.can_swap_block(first_decision_point.presented_block)){

            optional<Decision_point> second_decision_point =
                generate_child_decision_point(first_decision_point, placement, queue.cend(), placement_limit);

            if(!second_decision_point){
                continue;
            }

            State best_reachable_state_from_here = get_best_reachable_state(
                move(*second_decision_point), queue.cend(), placement_limit
            );
            if(best_reachable_state_from_here.has_greater_utility_than(best_foreseeable_state)){
                best_foreseeable_state = best_reachable_state_from_here;
                best_move = placement;
            }
        }

        if(!placement){
            break;
        }

    }
    return best_move;
}
