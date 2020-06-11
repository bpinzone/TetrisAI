#include "action.h"

#include "block.h"

#include <string>
#include <iostream>
#include <vector>

#include <thread>  // std::this_thread::sleep_for
#include <chrono>  // std::chrono::milliseconds

using std::ostream;
using std::string;
using std::vector;

Action::Action(const Block* block, Placement placement)
    : offset{placement.get_column() - block->maps[placement.get_rotation()].leftmost_block_pos},
    rotation_count{placement.get_rotation()},
    hold{placement.get_is_hold()} {
}

ostream& operator<<(ostream& os, const Action& action) {

    static const int c_action_delay_ms = 300;
    static const int c_wait_after_completion_ms = 500;
    vector<string> actions_strs;
    if(action.hold){
        actions_strs.push_back("r");
    }
    else{
        for(int i = 0; i < action.rotation_count; ++i){
            actions_strs.push_back("a");
        }
        const int right_presses = action.offset > 0 ? action.offset : 0;
        const int left_presses = action.offset < 0 ? -action.offset : 0;
        for(int i = 0; i < right_presses; ++i){
            actions_strs.push_back("right");
        }
        for(int i = 0; i < left_presses; ++i){
            actions_strs.push_back("left");
        }
        // potential problem: this could be wrong.
        actions_strs.push_back("up");
    }

    for(const auto& action : actions_strs){
        os << action;
        os << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(c_action_delay_ms));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(c_wait_after_completion_ms));
    

    return os;
}
