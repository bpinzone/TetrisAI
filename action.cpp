#include "action.h"

#include "block.h"

#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <iterator>

using std::ostream;
using std::endl;
using std::string;
using std::vector;
using std::back_inserter;
using std::copy;
using std::abs;

Action::Action(const Block* _block, Placement placement)
    : block{_block}, offset{placement.get_column() - block->maps[placement.get_rotation()].leftmost_block_pos},
    rotation_count{placement.get_rotation()},
    hold{placement.get_is_hold()}{
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

        vector<string> rot_strs;
        for(int i = 0; i < action.rotation_count; ++i){
            rot_strs.push_back("a");
        }

        vector<string> trans_strs;
        const int right_presses = action.offset > 0 ? action.offset : 0;
        const int left_presses = action.offset < 0 ? -action.offset : 0;
        for(int i = 0; i < right_presses; ++i){
            trans_strs.push_back("right");
        }
        for(int i = 0; i < left_presses; ++i){
            trans_strs.push_back("left");
        }

        // while rot count < trans count OR its safe to translate: single translate command
        const int trans_limit = action.offset > 0 ?
            action.block->safe_right_trans : action.block->safe_left_trans;

        for(int translations_done = 0;
                trans_strs.size() > rot_strs.size() && translations_done < trans_limit;
                ++translations_done){
            actions_strs.push_back(trans_strs.back());
            trans_strs.pop_back();
        }

        // Dance.
        // Assert: action_strs consists only of translation strings.
        // for(int dance_idx = 0; dance_idx < actions_strs.size() / 2; ++dance_idx){
        //     actions_strs[2 * dance_idx] += " && a";
        //     actions_strs[(2 * dance_idx) + 1] += " && b";
        // }

        // Combine rot and trans into same command while you can.
        while(!rot_strs.empty() && !trans_strs.empty()){
            actions_strs.push_back(rot_strs.back() + " && " + trans_strs.back());
            rot_strs.pop_back();
            trans_strs.pop_back();
        }
        copy(rot_strs.begin(), rot_strs.end(), back_inserter(actions_strs));
        copy(trans_strs.begin(), trans_strs.end(), back_inserter(actions_strs));

        if(!actions_strs.empty()){
            actions_strs.back() = actions_strs.back() + " && up";
        }
        else{
            actions_strs.push_back("up");
        }
    }

    for(const auto& action : actions_strs){
        os << action << endl; // must flush immediately.
    }
    return os;
}
