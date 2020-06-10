#include "board.h"
#include "state.h"
#include "block.h"
#include "action.h"
#include "utility.h"

#include <vector>
#include <deque>
#include <iostream>
#include <cassert>
#include <iterator>
#include <algorithm>
#include <string>
#include <utility>
#include <cstdlib>
#include <optional>
#include <mutex>
#include <thread>
#include <stdexcept>

using std::mutex;
using std::unique_lock;
using std::thread;

using std::swap;
using std::move;
using std::ref;

using std::cout;
using std::cin;
using std::endl;
using std::string;
using std::generate_n;
using std::back_inserter;
using std::max_element;
using std::optional;
using std::vector;
using std::runtime_error;

using Tetris_queue_t = State::Tetris_queue_t;
using Seed_t = Random_block_generator::Seed_t;

void play(Block_generator& block_generator, int queue_consume_lookahead_depth, int placements_to_perform, int max_queue_size);

Placement get_best_move(
        const Board& state,
        const Block& presented,
        const Tetris_queue_t& queue,
        const int num_placements_to_look_ahead);

void get_best_foreseeable_state_from_subtree(
    State&& seed_state, vector<State>& results, mutex& results_mutex);

// <prog> <seed> <lookahead_depth> <placements_to_perform> <max queue size>
int main(int argc, char* argv[]) {

    log_file.open(c_log_file_name);
    if(!log_file.is_open()){
        throw runtime_error{"Could not open log file for writing: " + string(c_log_file_name)};
    }
    if(argc != 5){
        log_file << "Incorrect usage: Requires: <seed or \"i\"> <lookahead depth> <placements to perform> <max queue size>" << endl;
        return 0;
    }

    Block_generator* block_generator;
    if(argv[1][0] == 'i'){
        block_generator = new Stdin_block_generator();
    }
    else{
        block_generator = new Random_block_generator(atoi(argv[1]));
    }

    play(*block_generator, atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));

    delete block_generator;
    log_file.close();
    return 0;
}

void play(Block_generator& block_generator, int num_placements_to_look_ahead, int placements_to_perform, int max_queue_size){

    if(num_placements_to_look_ahead > 7){
        log_file << "Jeff cannot see that far into the future. Enter 7 or less.\n";
        return;
    }
    if(num_placements_to_look_ahead < 1){
        log_file << "Jeff must be able to place a block!\n";
        return;
    }

    Board board;
    const Block* next_to_present = block_generator();
    Tetris_queue_t queue;
    for(int i = 0; i < max_queue_size; ++i){
        // Not going to write a wrapper just for this.
        // generate_n takes functor by value. Can't use inheritance.
        queue.push_back(block_generator());
    }

    int turn = 0;
    while(turn < placements_to_perform){

        // Status
        log_file << "Turn: " << turn << "\n";
        log_file << "Tetris percent:" << board.get_tetris_percent() << " %" << endl;
        log_file << "Presented with: " << next_to_present->name << endl;

        Placement next_placement = get_best_move(
            board, *next_to_present,
            queue, num_placements_to_look_ahead
        );

        cout << Action{next_to_present, next_placement};

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
                log_file << "Game over :(" << endl;
                return;
            }
            next_to_present = queue.front();
            queue.pop_front();
            queue.push_back(block_generator());
        }
        log_file << endl;
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

    State root_state{
        board,
        &presented,
        queue.cbegin(),
        false,
        queue.cend(),
        placement_limit,
        optional<Placement>{}
    };

    // Results destination
    vector<State> results;
    mutex results_mutex;

    vector<thread> workers;

    // Populate seeds for threads
    for(auto op_child = root_state.generate_next_child();
            op_child; op_child = root_state.generate_next_child()){

        workers.push_back(thread{
            &get_best_foreseeable_state_from_subtree,
            move(*op_child),
            ref(results),
            ref(results_mutex)
        });
    }

    for(auto& worker : workers){
        worker.join();
    }

    // Find the best
    return max_element(
        results.cbegin(), results.cend(),
        [](const auto& s1, const auto& s2){
            return s2.get_board().has_greater_utility_than(s1.get_board());
        }
    )->get_placement_taken_from_root();
}

void get_best_foreseeable_state_from_subtree(
        State&& seed_state, vector<State>& results, mutex& results_mutex){

    State best_foreseeable_state = State::generate_worst_state_from(seed_state);

    vector<State> state_stack {move(seed_state)};

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

    unique_lock<mutex> results_ulock{results_mutex};
    results.push_back(move(best_foreseeable_state));
}