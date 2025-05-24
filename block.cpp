#include "block.h"

#include "board.h"  // access in get_max_valid_placement_col
#include "board_size.h"

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
            "char_to_block_ptr: Invalid Block! "
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

    throw std::runtime_error{"block_ptr_to_char: Invalid block pointer!"};
}

const Color Block::char_to_color(char c){
    switch(c){
        case 'b': return Color::Blue;
        case 'p': return Color::Purple;
        case 'r': return Color::Red;
        case 'c': return Color::Cyan;
        case 'y': return Color::Yellow;
        case 'o': return Color::Orange;
        case 'g': return Color::Green;
        default: throw std::logic_error{"char_to_color: color char invalid!"};
    }
}

const char Block::color_to_char(Color color){
    switch(color){
        case Color::Blue: return 'b';
        case Color::Purple: return 'p';
        case Color::Red: return 'r';
        case Color::Cyan: return 'c';
        case Color::Yellow: return 'y';
        case Color::Orange: return 'o';
        case Color::Green: return 'g';
        default: throw std::logic_error{"color_to_char: Color enum value invalid!"};
    }
}

std::string Block::name_to_full_name(char c){
    switch(c){
        case 'b': return "Blue";
        case 'p': return "Purple";
        case 'r': return "Red";
        case 'c': return "Cyan";
        case 'y': return "Yellow";
        case 'o': return "Orange";
        case 'g': return "Green";
        default: throw std::logic_error{"name_to_full_name: color char invalid!"};
    }
}



int Block::get_max_valid_placement_col(int rot_x) const {
    return BoardSize::c_cols - maps[rot_x].contour.size();
}

Block::Block(char _name, const vector<CH_maps>& _maps)
    : name{_name}, maps{_maps} {

    color = char_to_color(_name);
    safe_left_trans = maps[0].leftmost_block_pos;
    safe_right_trans = BoardSize::c_cols - (maps[0].leftmost_block_pos + maps[0].contour.size());
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
const Block Block::Blue {'b', {
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

const Block Block::Purple {'p', {

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

const Block Block::Red {'r', {
    // XX
    //  XX
    { {0, -1, -1}, {1, 2, 1}, 3},

    //  X
    // XX
    // X
    { {0, 1}, {2, 2}, 4},
}};

const Block Block::Cyan {'c', {
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

const Block Block::Yellow {'y', {
    // XX
    // XX
    { {0, 0}, {2, 2}, 4 },
}};

const Block Block::Orange {'o', {

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

const Block Block::Green {'g', {

    //  XX
    // XX
    { {0, 0, 1}, {1, 2, 1}, 3 },

    // X
    // XX
    //  X
    { {0, -1}, {2, 2}, 4},
}};
