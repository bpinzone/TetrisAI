#include "human_action.h"

#include "block.h"

#include <string>
#include <cstdio>

using std::ostream;
using std::endl;
using std::string;
using std::to_string;

HumanAction::HumanAction(const Block* block, Placement placement)
    : offset{placement.get_column() - block->maps[placement.get_rotation()].leftmost_block_pos},
    rotation_count{placement.get_rotation()},
    hold{placement.get_is_hold()} {
}

void HumanAction::print_to_stdout() const{

    string hold_instruction = hold ? "HOLD" : "    ";

    bool has_left_instruction = offset < 0 && !hold;
    string left_button_instruction = has_left_instruction ? to_string(-offset) : " ";

    bool has_right_instruction = offset > 0 && !hold;
    string right_button_instruction = has_right_instruction ? to_string(offset) : " ";

    bool has_rotation_instruction = rotation_count != 0 && !hold;
    string rotation_instruction = has_rotation_instruction ? to_string(rotation_count) : " ";

    printf(
        "\n\n************************************************************\n"
        "************************************************************\n"
        "*************   ***********      ***********   *************\n"
        "************* %s *********** %s *********** %s *************\n"
        "*************   ***********      ***********   *************\n"
        "************************************************************\n"
        "****************************    ****************************\n"
        "**************************** %s  ****************************\n"
        "****************************    ****************************\n"
        "************************************************************\n\n\n",
        left_button_instruction.c_str(), hold_instruction.c_str(), right_button_instruction.c_str(), rotation_instruction.c_str()
    );

}