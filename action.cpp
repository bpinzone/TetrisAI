#include "action.h"

#include "block.h"

#include <iostream>
#include <vector>

#include <thread>  // std::this_thread::sleep_for
#include <chrono>  // std::chrono::milliseconds

using std::ostream;
using std::string;
using std::vector;
using std::optional;

Action::Action(const Block* block, Placement placement, optional<string> _wait_command_to_follow)
    : offset{placement.get_column() - block->maps[placement.get_rotation()].leftmost_block_pos},
    rotation_count{placement.get_rotation()},
    hold{placement.get_is_hold()},
    wait_command_to_follow{_wait_command_to_follow} {
}

ostream& operator<<(ostream& os, const Action& action) {

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

    if(action.wait_command_to_follow){
        actions_strs.push_back(*action.wait_command_to_follow);
    }

    for(const auto& action : actions_strs){
        os << action;
        os << std::endl;
    }
    return os;
}
