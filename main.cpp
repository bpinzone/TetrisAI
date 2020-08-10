#include "board.h"
#include "state.h"
#include "block.h"
#include "action.h"
#include "utility.h"
#include "tetris_worker.h"
#include "play_settings.h"

#include <iostream>
#include <cassert>
#include <iterator>
#include <algorithm>
#include <string>
#include <utility>
#include <optional>

using std::swap;
using std::move;
using std::endl;
using std::cin;
using std::string;
using std::back_inserter;
using std::transform;
using std::optional;

using Tetris_queue_t = State::Tetris_queue_t;
using Seed_t = Random_block_generator::Seed_t;

void play(const Play_settings& settings);
void play_99(const Play_settings& settings);

Placement get_best_move(
        const Board& board,
        const Block& presented,
        const Tetris_queue_t& queue,
        const int num_placements_to_look_ahead);

int main(int argc, char* argv[]) {

    // For Xcode file redirection.
#ifdef __APPLE__
   if (getenv("STDIN"))
      freopen(getenv("STDIN"), "r", stdin);
   if (getenv("STDOUT"))
      freopen(getenv("STDOUT"), "w", stdout);
   if (getenv("STDERR"))
      freopen(getenv("STDERR"), "w", stderr);
#endif

    Play_settings ps(argc, argv);
    Output_manager::get_instance().set_streams(ps.mode);
    ps.wait_for_controller_connection_if_necessary();

    Tetris_worker::create_workers(ps.num_threads);
    // Wait for them to spin up and mark themselves free for work.
    Tetris_worker::wait_until_all_free();

    if(ps.is_watching()){
        play(ps);
    }
    else{
        play_99(ps);
    }

    // Not bothering with unique_ptr
    delete ps.block_generator;
    return 0;
}

void play(const Play_settings& settings){

    Board board;
    const Block* next_to_present = settings.block_generator->generate();
    Tetris_queue_t queue;
    for(int i = 0; i < settings.queue_size; ++i){
        // Not going to write a wrapper just for this.
        // generate_n takes functor by value. Can't use bc we use inheritance. Use ref()?
        queue.push_back(settings.block_generator->generate());
    }

    int turn = 0;
    while(turn < settings.game_length){

        // Status
        Output_manager::get_instance().get_board_os()
            << "Turn: " << turn << "\n"
            << "Tetris percent:" << board.get_tetris_percent() << " %" << "\n"
            << "Presented with: " << next_to_present->name << "\n";

        Placement next_placement = get_best_move(
            board, *next_to_present,
            queue, settings.lookahead_placements
        );

        Board new_board{board};

        if(!settings.is_watching()){
            Action action{next_to_present, next_placement};
            Output_manager::get_instance().get_command_os() << action;
        }

        // HOLD
        if(next_placement.get_is_hold()){

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

            if(!is_promising){
                Output_manager::get_instance().get_board_os() << "Game over :(" << endl;
                return;
            }
            next_to_present = queue.front();
            queue.pop_front();
            queue.push_back(settings.block_generator->generate());
        }

        swap(board, new_board);
        Output_manager::get_instance().get_board_os() << "\n" << board << endl;
        ++turn;
    }
}

// TODO: Poor design. Got messy after adding double plays because of holds, and board overrides.
void play_99(const Play_settings& settings) {

    optional<Board> override_board;
    Board_lifetime_stats lifetime_stats;
    while(true){

        // Read in effective state.
        string label;
        cin >> label;
        assert(label == "presented");
        char presented_ch;
        cin >> presented_ch;
        const Block* presented = Block::char_to_block_ptr(presented_ch);

        cin >> label;
        assert(label == "queue");
        string queue_chars;
        cin >> queue_chars;
        Tetris_queue_t queue;
        transform(queue_chars.begin(), queue_chars.end(), back_inserter(queue), &Block::char_to_block_ptr);

        Board board(cin);
        if(override_board){
            board = *override_board;
            override_board = {};
        }
        board.set_lifetime_stats(lifetime_stats);

        // Output info.
        Output_manager::get_instance().get_board_os()
            << "C++ thinks the queue is: " << queue_chars << endl
            << " ====================== " << endl
            << "C++ thinks the board is: " << endl
            << board << endl;

        // Compute placement
        Placement next_placement = get_best_move(
            board, *presented, queue, settings.lookahead_placements);

        // Send Command
        Action action{presented, next_placement};
        Output_manager::get_instance().get_command_os() << action;

        bool just_held_non_first = false;
        // Predict Future
        Board new_board{board};
        if(next_placement.get_is_hold()){
            const Block* old_hold = new_board.swap_block(*presented);
            if(old_hold){
                just_held_non_first = true;
                board = new_board;
                presented = old_hold;
            }
        }
        else{
            new_board.place_block(*presented, next_placement);
            if(new_board.get_lifetime_stats().num_placements_that_cleared_rows
                    > board.get_lifetime_stats().num_placements_that_cleared_rows){
                override_board = new_board;
            }
        }
        lifetime_stats = new_board.get_lifetime_stats();
        Output_manager::get_instance().get_board_os()
            << endl << "C++ thinks after the move, the board will be: " << endl << new_board << endl
            << " ====================== " << endl;


        // If we ever hold, just take the next move right away, not waiting for another state to come through the pipeline.

        if(!just_held_non_first){
            continue;
        }

        // === BONUS TIME ===

        Output_manager::get_instance().get_board_os()
            << endl << "C++ just held!: " << endl;

        next_placement = get_best_move(
            board, *presented, queue, settings.lookahead_placements);

        // Predict Future
        // You could assert new_board == board
        assert(!next_placement.get_is_hold());
        new_board.place_block(*presented, next_placement);

        if(new_board.get_lifetime_stats().num_placements_that_cleared_rows
                > board.get_lifetime_stats().num_placements_that_cleared_rows){
            override_board = new_board;
        }

        lifetime_stats = new_board.get_lifetime_stats();
        Output_manager::get_instance().get_board_os()
            << endl << "C++ thinks after the BONUS BONUS BONUS move, the board will be: " << endl << new_board << endl
            << " ====================== " << endl;

        // Send Command
        action = Action{presented, next_placement};
        Output_manager::get_instance().get_command_os() << action;
    }
}

Placement get_best_move(
        const Board& board,
        const Block& presented,
        const Tetris_queue_t& queue,
        const int num_placements_to_look_ahead){

    Tetris_worker::assert_all_free();

    State root_state = State::generate_root_state(
        board, presented, queue, num_placements_to_look_ahead);

    Tetris_worker::distribute_new_work_and_wait_till_all_free(move(root_state));

    State& best_state = Tetris_worker::get_best_reachable_state();
    return best_state.get_placement_taken_from_root();
}
