#include "board.h"
#include "state.h"
#include "block.h"
#include "action.h"
#include "utility.h"
#include "tetris_worker.h"
#include "play_settings.h"
#include "global_stats.h"
#include "post_play.h"

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
using std::cout;
using std::string;
using std::back_inserter;
using std::transform;
using std::optional;

using Tetris_queue_t = State::Tetris_queue_t;
using Seed_t = Random_block_generator::Seed_t;

void play(const Play_settings& settings);
void play_99(const Play_settings& settings);
Post_play_report play_99_move(Game_state& original_state, const Play_settings& settings);

Placement get_best_move(
        Board& board,
        const Block& presented,
        const Tetris_queue_t& queue,
        int num_placements_to_look_ahead);

// struct minimal_lifetime_stats {
//     float max_height_exp_moving_average;
//     short num_blocks_placed;
//     short num_placements_that_cleared_rows;
//     short num_tetrises;
//     short num_non_tetrises;
//     char num_all_clears;
// };

// struct minimal_ancestor_data {
//     char highest_height;
//     char second_lowest_height;
//     bool good_trench_status;
// };

// // struct minimal_state
// struct minimal_board {
//     static constexpr size_t c_cols = 10;
//     static constexpr size_t c_rows = 20;
//     static constexpr size_t c_size = c_cols * c_rows;
//     using Grid_t = std::bitset<c_size>;

//     Grid_t board;
//     minimal_lifetime_stats mls;
//     std::array<char, c_cols> height_map;
//     minimal_ancestor_data mad;

//     int sum_of_squared_heights;

//     char lowest_height, sec_lowest_height, highest_height;

//     char current_hold;
//     char num_cells_filled;
//     char perfect_num_cells_filled;
//     char num_trenches;

//     bool at_least_one_side_clear;
//     bool is_tetrisable;
//     bool just_swapped;

// };

// struct minimal_placement_generator {
//     char presented;
//     char rot_x;
//     char col;
//     bool exhaused;
// };

// struct minimal_placement {
//     char rot, col, is_hold;
// };

// struct minimal_state {
//     minimal_board b;
//     char queue[6];
//     minimal_placement_generator pg;
//     minimal_placement ms;
//     char presented_block;
//     char next_queue_it;
//     char end_queue_it;
//     char placement_limit;
//     bool is_leaf;

// };

int main(int argc, char* argv[]) {

    // std::cout << "state bytes: " << sizeof(minimal_state) << std::endl;
    // std::cout << "min lifetime stats: " << sizeof(minimal_lifetime_stats) << std::endl;
    // std::cout << "min_anc: " << sizeof(minimal_ancestor_data) << std::endl;
    // std::cout << "board : " << sizeof(std::bitset<200>) << std::endl;
    // std::cout << "height map : " << sizeof(std::array<char, 10>) << std::endl;
    // // std::cout << "state bytes: " << sizeof(State) << std::endl;
    // return 0;

    std::ios_base::sync_with_stdio(false);

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
    // ps.wait_for_controller_connection_if_necessary();

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

        cout << "Comparisons: " << gs_num_comparisons << endl;

        // Status
        if(settings.board_log){
            Output_manager::get_instance().get_board_os()
                << "Turn: " << turn << "\n"
                << "Tetris percent:" << board.get_tetris_percent() << " %" << "\n"
                << "Presented with: " << next_to_present->name << "\n";
        }

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
                if(settings.board_log){
                    Output_manager::get_instance().get_board_os() << "Game over :(" << endl;
                }
                return;
            }
            next_to_present = queue.front();
            queue.pop_front();
            queue.push_back(settings.block_generator->generate());
        }

        swap(board, new_board);
        if(settings.board_log){
            Output_manager::get_instance().get_board_os() << "\n" << board << endl;
        }
        ++turn;
    }

    cout << "Comparisons per turn: " << static_cast<double>(gs_num_comparisons) / settings.game_length << endl;
}


void play_99(const Play_settings& settings) {

    optional<Post_play_report> post_play_report;
    Board_lifetime_stats lifetime_stats;

    while(true){

        char buffer;
        cin >> buffer;
        if(buffer == '!'){
            Action::switch_to_attackers = true;
            // Do manual command.
            string command;
            getline(cin, command, '!');
            cout << command << endl;
            continue;
        }
        else{
            cin.putback(buffer);
        }

        Vision_state vision_state{cin};

        bool replace_board = post_play_report && (
            post_play_report->just_cleared_line || vision_state.use_last_resulting_board
        );

        if(replace_board){
            vision_state.game_state.board = post_play_report->board;
        }

        vision_state.game_state.board.set_lifetime_stats(lifetime_stats);
        post_play_report = play_99_move(vision_state.game_state, settings);
        lifetime_stats = post_play_report->board.get_lifetime_stats();

        if(post_play_report->just_held_non_first){

            if(settings.board_log){
                Output_manager::get_instance().get_board_os()
                    << endl << "C++ just held!: " << endl;
            }

            Game_state post_hold_game_state{
                post_play_report->presented,
                vision_state.game_state.queue,
                post_play_report->board
            };

            post_play_report = play_99_move(post_hold_game_state, settings);
            lifetime_stats = post_play_report->board.get_lifetime_stats();
        }

    }
}

// Original_state is what the c++ will actually act on.
// Assumes: At call time, original_state's lifetime stats are up to date.
Post_play_report play_99_move(Game_state& original_state, const Play_settings& settings){

    string queue_str;
    transform(
        original_state.queue.begin(), original_state.queue.end(),
        back_inserter(queue_str),
        &Block::block_ptr_to_char
    );

    // Output info.
    if(settings.board_log){
        Output_manager::get_instance().get_board_os()
            << "C++ thinks the queue is: " << queue_str << "\n"
            << "C++ thinks the presented is: "
            << Block::block_ptr_to_char(original_state.presented) << "\n"
            << " ====================== " << "\n"
            << "C++ thinks the board is: " << "\n"
            << original_state.board << "\n";
    }

    // Compute placement
    Placement next_placement = get_best_move(
        original_state.board,
        *original_state.presented,
        original_state.queue,
        settings.lookahead_placements);

    if(settings.board_log){
        Output_manager::get_instance().get_board_os() << "Time to press buttons:\n";
    }

    // Send Command
    // If its a hold, presented is not read by action or its members.
    Action action{original_state.presented, next_placement};
    Output_manager::get_instance().get_command_os() << action;
    if(settings.board_log){
        Output_manager::get_instance().get_board_os() << action;
    }

    bool just_held_non_first = false;
    bool just_cleared_line = false;
    const Block* new_presented_block = nullptr;

    // Predict Future
    Board new_board{original_state.board};
    if(next_placement.get_is_hold()){
        const Block* old_hold = new_board.swap_block(*original_state.presented);
        if(old_hold){
            just_held_non_first = true;
            new_presented_block = old_hold;
        }
    }
    else{
        new_board.place_block(*original_state.presented, next_placement);
        new_presented_block = nullptr; // this should never be read.
        // last_resulting_board = new_board;
        // If you cleared a row, DON'T read next time.
        if(new_board.get_lifetime_stats().num_placements_that_cleared_rows
                > original_state.board.get_lifetime_stats().num_placements_that_cleared_rows){
            just_cleared_line = true;
        }
    }

    if(settings.board_log){
        Output_manager::get_instance().get_board_os()
            << endl << "C++ thinks after the move, the board will be: " << endl << new_board << endl
            << " ====================== " << endl;
    }

    return Post_play_report{
        new_presented_block,
        new_board,
        just_held_non_first,
        just_cleared_line
    };
}

Placement get_best_move(
        Board& board,
        const Block& presented,
        const Tetris_queue_t& queue,
        int num_placements_to_look_ahead){


    board.load_ancestral_data_with_current_data();

    Tetris_worker::assert_all_free();

    State root_state = State::generate_root_state(
        board, presented, queue, num_placements_to_look_ahead);

    Tetris_worker::distribute_new_work_and_wait_till_all_free(move(root_state));

    State& best_state = Tetris_worker::get_best_reachable_state();

    // Tetris_worker::print_workers_states();

    // cout << "This is the worst board I can imagine!\n";
    // cout << best_state << "\n";

    return best_state.get_placement_taken_from_root();
}

