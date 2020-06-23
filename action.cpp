#include "action.h"

#include "block.h"

#include <iostream>
#include <vector>

using std::ostream;
using std::string;
using std::vector;

Action::Action(const Block* block, Placement placement, const string& _wait_command_to_follow)
    : offset{placement.get_column() - block->maps[placement.get_rotation()].leftmost_block_pos},
    rotation_count{placement.get_rotation()},
    hold{placement.get_is_hold()},
    wait_command_to_follow{_wait_command_to_follow} {
}

ostream& operator<<(ostream& os, const Action& action) {

    static bool is_first_action = true;

    vector<string> actions_strs;
    if(is_first_action){
        is_first_action = false;
        actions_strs.push_back("stick r down");
        actions_strs.push_back("wait");
        actions_strs.push_back("stick r center");
    }

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
        actions_strs.push_back("up");
    }

    actions_strs.push_back(action.wait_command_to_follow);

    for(const auto& action : actions_strs){
        os << action;
        os << std::endl; // flush immediately.
    }
    return os;
}
