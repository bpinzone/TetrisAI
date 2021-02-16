#include "post_play.h"

#include <string>
#include <iostream>
#include <cassert>
#include <algorithm>
#include <iterator>

using std::string;
using std::istream;
using std::cin;
using std::transform;
using std::back_inserter;
using std::queue;

Vision_state::Vision_state(istream& is)

    : game_state(is) {

    string label;
    is >> label;
    assert(label == "board_obscured");

    string use_last_resulting_board_str;
    is >> use_last_resulting_board_str;
    use_last_resulting_board = use_last_resulting_board_str == "true";
}

Game_state::Game_state(istream& is){

    // Read in effective state.
    string label;

    is >> label;
    assert(label == "presented");
    char presented_ch;
    is >> presented_ch;
    presented = Block::char_to_block_ptr(presented_ch);

    is >> label;
    assert(label == "queue");
    string queue_chars;
    is >> queue_chars;
    transform(queue_chars.begin(), queue_chars.end(), back_inserter(queue), &Block::char_to_block_ptr);

    board = Board(is);
}