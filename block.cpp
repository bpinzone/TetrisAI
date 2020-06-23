#include "block.h"

#include "board.h"  // access in get_max_valid_placement_col

#include <iostream>
#include <stdexcept>

using std::uniform_int_distribution;
using std::default_random_engine;
using std::string;
using std::vector;
using std::cin;
using std::runtime_error;

int Block::get_max_valid_placement_col(int rot_x) const {
    return Board::c_cols - maps[rot_x].contour.size();
}

Block::Block(const string& _name, const vector<CH_maps>& _maps)
    : name{_name}, maps{_maps} {
}

// === Block Generators ===

const Block* Stdin_block_generator::generate() {
    char block;
    cin >> block;
    assert(cin);
    switch(block){
        case 'b' : return &Block::Blue;
        case 'p' : return &Block::Purple;
        case 'r' : return &Block::Red;
        case 'c' : return &Block::Cyan;
        case 'y' : return &Block::Yellow;
        case 'o' : return &Block::Orange;
        case 'g' : return &Block::Green;
        default : throw std::runtime_error{"Invalid Block! " + string{block} + " Enter one of: b, p, r, c, y, o, g."};
    }
}

Random_block_generator::Random_block_generator(Seed_t seed)
    : generator{seed} {
}

const Block* Random_block_generator::generate() {
    return block_ptrs[distribution(generator)];
}

const Block* Random_block_generator::block_ptrs[] = {
    &Block::Cyan, &Block::Blue, &Block::Orange,
    &Block::Green, &Block::Red, &Block::Yellow, &Block::Purple
};

uniform_int_distribution<int> Random_block_generator::distribution{
    0, (sizeof(block_ptrs) / sizeof(block_ptrs[0])) - 1
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
