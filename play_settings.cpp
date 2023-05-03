#include "play_settings.h"

#include "block.h"
#include "utility.h"

#include <stdexcept>
#include <thread>
#include <chrono>
#include <string>
#include <cstdlib>
#include <iostream>

using std::endl;
using std::string;
using std::runtime_error;

Play_settings::Play_settings(int argc, char* argv[]){

    if(argc != num_settings + 1){
        std::cout <<
            "Usage: <mode: wsmr> <block generation: seed# or i> <lookahead> <queue size> <game length> <num threads> <e to see board log, anything else otherwise>"
            "\n"
            "For example: ./main w 0 7 6 100 20 e"
            << endl;;

        exit(1);
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
    num_threads = atoi(argv[6]);
    board_log = string(argv[7]) == "e";

    if(lookahead_placements > queue_size + 1 ){
        throw runtime_error{"With this queue size, Jeff cannot see that far into the future"};
    }
    if(lookahead_placements < 1){
        throw runtime_error{"Jeff needs something to work with here!"};
    }

}

void Play_settings::wait_for_controller_connection_if_necessary(){
    if(is_watching() || mode == 's'){
        return;
    }
    if(mode != 'm' && mode != 'r'){
        throw runtime_error{"Invalid mode"};
    }

    std::this_thread::sleep_for(std::chrono::seconds(20));
    Output_manager::get_instance().get_command_os() << "l && r" << endl;

    std::this_thread::sleep_for(std::chrono::seconds(10));
    Output_manager::get_instance().get_command_os() << "a" << endl;

    std::this_thread::sleep_for(std::chrono::seconds(15));
    string wait_command = mode == 'm' ? "a" : "hold_a";
    Output_manager::get_instance().get_command_os() << wait_command;
    Output_manager::get_instance().get_command_os() << endl;
}