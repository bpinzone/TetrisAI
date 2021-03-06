#include "block.h"

#include "board.h"  // access in get_max_valid_placement_col

#include <iostream>
#include <stdexcept>
#include <algorithm>

using std::shuffle;
using std::string;
using std::vector;
using std::cin;
using std::runtime_error;

const Block* Block::char_to_block_ptr(char c){

    switch(c){
        case 'b' : return &Blue;
        case 'p' : return &Purple;
        case 'r' : return &Red;
        case 'c' : return &Cyan;
        case 'y' : return &Yellow;
        case 'o' : return &Orange;
        case 'g' : return &Green;
        default : throw std::runtime_error{
            "Invalid Block! "
            + string{c}
            + " Must be one of: b, p, r, c, y, o, g."
        };
    }
}

const char Block::block_ptr_to_char(const Block* block){

    if(block == &Blue){
        return 'b';
    }
    if(block == &Purple){
        return 'p';
    }
    if(block == &Red){
        return 'r';
    }
    if(block == &Cyan){
        return 'c';
    }
    if(block == &Yellow){
        return 'y';
    }
    if(block == &Orange){
        return 'o';
    }
    if(block == &Green){
        return 'g';
    }

    throw std::runtime_error{"Invalid block pointer!"};
}

int Block::get_max_valid_placement_col(int rot_x) const {
    return Board::c_cols - maps[rot_x].contour.size();
}

Block::Block(const string& _name, const vector<CH_maps>& _maps)
    : name{_name}, maps{_maps} {

    safe_left_trans = maps[0].leftmost_block_pos;
    safe_right_trans = Board::c_cols - (maps[0].leftmost_block_pos + maps[0].contour.size());
}

// === Block Generators ===

const Block* Stdin_block_generator::generate() {
    char block;
    cin >> block;
    return Block::char_to_block_ptr(block);
}

Random_block_generator::Random_block_generator(Seed_t seed)
    : generator{seed} {
}

const Block* Random_block_generator::generate() {

    if(bag_instance.empty()){
        shuffle(full_bag.begin(), full_bag.end(), generator);
        for(const auto& b : full_bag){
            bag_instance.push(b);
        }
    }
    const auto block = bag_instance.front();
    bag_instance.pop();
    return block;
}

vector<const Block*> Random_block_generator::full_bag {
    &Block::Cyan, &Block::Blue, &Block::Orange,
    &Block::Green, &Block::Red, &Block::Yellow, &Block::Purple
};

/*
Example:

XX
 XX

contour: 0, -1, -1
height: 1, 2, 1
*/
const Block Block::Blue {"Blue", {
    // X
    // XXX
    { {0, 0, 0}, {2, 1, 1}, 3},

    // XX
    // X
    // X
    { {0, 2}, {3, 1}, 4},

    // XXX
    //   X
    { {0, 0, -1}, {1, 1, 2}, 3},

    //  X
    //  X
    // XX
    { {0, 0}, {1, 3}, 3},
}};

const Block Block::Purple {"Purple", {

    //  X
    // XXX
    { {0, 0, 0}, {1, 2, 1}, 3},

    // X
    // XX
    // X
    { {0, 1}, {3, 1}, 4},

    // XXX
    //  X
    { {0, -1, 0}, {1, 2, 1}, 3},

    //  X
    // XX
    //  X
    { {0, -1}, {1, 3}, 3},
}};

const Block Block::Red {"Red", {
    // XX
    //  XX
    { {0, -1, -1}, {1, 2, 1}, 3},

    //  X
    // XX
    // X
    { {0, 1}, {2, 2}, 4},
}};

const Block Block::Cyan {"Cyan", {
    // XXXX
    {
        {0, 0, 0, 0}, // Contour
        {1, 1, 1, 1}, // Height
        3
    },
    // X
    // X
    // X
    // X
    {
        {0}, // Contour
        {4}, // Height
        5
    },
}};

const Block Block::Yellow {"Yellow", {
    // XX
    // XX
    { {0, 0}, {2, 2}, 4 },
}};

const Block Block::Orange {"Orange", {

    //   X
    // XXX
    { {0, 0, 0}, {1, 1, 2}, 3},

    // X
    // X
    // XX
    { {0, 0}, {3, 1}, 4},

    // XXX
    // X
    { {0, 1, 1}, {2, 1, 1}, 3},

    // XX
    //  X
    //  X
    { {0, -2}, {1, 3}, 3},

}};

const Block Block::Green {"Green", {

    //  XX
    // XX
    { {0, 0, 1}, {1, 2, 1}, 3 },

    // X
    // XX
    //  X
    { {0, -1}, {2, 2}, 4},
}};
