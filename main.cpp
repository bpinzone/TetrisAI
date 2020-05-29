#include "board.h"
#include "state.h"
#include "block.h"

#include <vector>
#include <deque>
#include <iostream>
#include <cassert>
#include <iterator>
#include <algorithm>
#include <utility>
#include <cstdlib>
#include <optional>

using std::cout;
using std::endl;
using std::swap;
using std::move;
using std::generate_n;
using std::back_inserter;
using std::optional;
using std::vector;

using Tetris_queue_t = State::Tetris_queue_t;
using Seed_t = Block_generator::Seed_t;

void play(Seed_t seed, int queue_consume_lookahead_depth, int placements_to_perform);

Placement get_best_move(
        const Board& state,
        const Block& presented,
        const Tetris_queue_t& queue,
        const int num_placements_to_look_ahead);

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

    Board board;
    const Block* next_to_present = block_generator();
    Tetris_queue_t queue;
    generate_n(back_inserter(queue), 6, block_generator);

    int turn = 0;
    while(turn < placements_to_perform){

        // Status
        cout << "Turn: " << turn << "\n";
        cout << "Tetris percent:" << board.get_tetris_percent() << " %" << endl;
        cout << "Presented with: " << next_to_present->name << endl;

        Placement next_placement = get_best_move(
            board, *next_to_present,
            queue, num_placements_to_look_ahead
        );

        Board new_board{board};
        // HOLD
        if(next_placement.get_is_hold()){
            const Block* old_hold = new_board.swap_block(*next_to_present);
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
            bool is_promising = new_board.place_block(*next_to_present, next_placement);
            if(!is_promising){
                cout << "Game over :(" << endl;
                return;
            }
            next_to_present = queue.front();
            queue.pop_front();
            queue.push_back(block_generator());
        }
        board.print_diff_against(new_board);
        cout << endl;
        swap(board, new_board);
        ++turn;
    }
}

Placement get_best_move(
        const Board& board,
        const Block& presented,
        const Tetris_queue_t& queue,
        const int num_placements_to_look_ahead){

    const int placement_limit = board.get_num_blocks_placed() + num_placements_to_look_ahead;

    State best_foreseeable_state{
        Board::get_worst_board(),
        &presented,
        queue.cbegin(),
        false,
        queue.cend(),
        placement_limit,
        optional<Placement>{{0, 0, false}}
    };

    // vector<int> v{{{}}};

    // Put the root state into the stack.
    vector<State> state_stack {{
        board,
        &presented,
        queue.cbegin(),
        false,
        queue.cend(),
        placement_limit,
        optional<Placement>{}
    }};

    while(!state_stack.empty()){

        State considered_state = move(state_stack.back());
        state_stack.pop_back();

        if(considered_state.get_is_leaf()){
            if(considered_state.get_board().has_greater_utility_than(best_foreseeable_state.get_board())){
                best_foreseeable_state = move(considered_state);
            }
        }
        else{
            for(auto op_child = considered_state.generate_next_child();
                    op_child; op_child = considered_state.generate_next_child()){
                state_stack.push_back(move(*op_child));
            }
        }
    }

    return best_foreseeable_state.get_placement_taken_from_root();
}
