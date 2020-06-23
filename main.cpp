#include "board.h"
#include "state.h"
#include "block.h"
#include "action.h"
#include "utility.h"

#include <vector>
#include <deque>
#include <iostream>
#include <cassert>
#include <chrono>
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

// ALL gathered from command line.
struct Play_settings {

    /*
    w: watch jeff play in terminal
    s: 99 skip
    m: 99 from menu
    r: 99 from restart
    */
    char mode;

    /*
    <seed int> OR i (stands for input)
    */
    Block_generator* block_generator;

    // [1, 7]
    int lookahead_placements;

    // [0, 6] (check this)
    int queue_size;

    int game_length;

    // <mode: wsmr> <generation: seed OR i> <lookahead> <queue size> <game length>
    Play_settings(int argc, char* argv[]){

        static int num_settings = 5;

        if(argc != num_settings + 1){
            throw runtime_error("Usage: <mode: wsmr> <block generation: seed# or i> <lookahead> <queue size> <game length>");
        }

        mode = *argv[1];
        if(*argv[2] == 'i'){
            block_generator = new Stdin_block_generator();
        }
        else{
            block_generator = new Random_block_generator(atoi(argv[2]));
        }

        lookahead_placements = atoi(argv[3]);
        queue_size = atoi(argv[4]);
        game_length = atoi(argv[5]);

        if(lookahead_placements > queue_size + 1 ){
            throw runtime_error{"With this queue size, Jeff cannot see that far into the future"};
        }
        if(lookahead_placements < 1){
            throw runtime_error{"Jeff needs something to work with here!"};
        }

    }

    void wait_as_necessary(){
        if(mode == 'w' || mode == 's'){
            return;
        }
        if(mode != 'm' && mode != 'r'){
            throw runtime_error{"Invalid mode"};
        }

        std::this_thread::sleep_for(std::chrono::seconds(20));
        Output_manager::get_instance().get_command_os() << "l && r" << endl;

        std::this_thread::sleep_for(std::chrono::seconds(10));
        Output_manager::get_instance().get_command_os() << "a" << endl;

        std::this_thread::sleep_for(std::chrono::seconds(10));
        string wait_command = mode == 'm' ? "a" : "hold_a";
        Output_manager::get_instance().get_command_os() << wait_command;
        Output_manager::get_instance().get_command_os() << endl;
    }
};

void play(const Play_settings& settings);

Placement get_best_move(
        const Board& state,
        const Block& presented,
        const Tetris_queue_t& queue,
        const int num_placements_to_look_ahead);

void get_best_foreseeable_state_from_subtree(
    State&& seed_state, vector<State>& results, mutex& results_mutex);

// <prog> <seed> <lookahead_depth> <placements_to_perform> <max queue size> <menu (m) or restart (r) or skip (s)>
int main(int argc, char* argv[]) {

    Play_settings ps(argc, argv);
    Output_manager::get_instance().set_streams(ps.mode);
    ps.wait_as_necessary();

    play(ps);

    delete ps.block_generator;
    return 0;
}

void play(const Play_settings& settings){

    Board board;
    const Block* next_to_present = settings.block_generator->generate();
    Tetris_queue_t queue;
    for(int i = 0; i < settings.queue_size; ++i){
        // Not going to write a wrapper just for this.
        // generate_n takes functor by value. Can't use inheritance.
        queue.push_back(settings.block_generator->generate());
    }

    int turn = 0;
    while(turn < settings.game_length){

        // Status
        Output_manager::get_instance().get_board_os() << "Turn: " << turn << "\n";
        Output_manager::get_instance().get_board_os() << "Tetris percent:" << board.get_tetris_percent() << " %" << endl;
        Output_manager::get_instance().get_board_os() << "Presented with: " << next_to_present->name << endl;

        Placement next_placement = get_best_move(
            board, *next_to_present,
            queue, settings.lookahead_placements
        );

        Board new_board{board};
        // HOLD
        if(next_placement.get_is_hold()){

            Action action{next_to_present, next_placement, "wait"};
            Output_manager::get_instance().get_command_os() << action;

            const Block* old_hold = new_board.swap_block(*next_to_present);
            if(!old_hold){
                next_to_present = queue.front();
                queue.pop_front();
                queue.push_back(settings.block_generator->generate());
            }
            else{
                next_to_present = old_hold;
            }
        }
        // PLACE
        else{
            bool is_promising = new_board.place_block(*next_to_present, next_placement);

            Action action{
                next_to_present, next_placement,
                new_board.has_more_cleared_rows_than(board) ? "wait_long" : "wait" };
            Output_manager::get_instance().get_command_os() << action;

            if(!is_promising){
                Output_manager::get_instance().get_board_os() << "Game over :(" << endl;
                return;
            }
            next_to_present = queue.front();
            queue.pop_front();
            queue.push_back(settings.block_generator->generate());
        }

        swap(board, new_board);
        Output_manager::get_instance().get_board_os() << endl << board << endl;
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

    vector<State> state_stack;
    state_stack.push_back(move(seed_state));

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
