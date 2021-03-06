#ifndef BLOCK_H
#define BLOCK_H

#include <string>
#include <vector>
#include <queue>
#include <cassert>
#include <random>

// For every shape block, and for every rotation, one of these exists to descripe the shape of the block in that orientation.
struct CH_maps {
    std::vector<int> contour;
    std::vector<int> height;
    // TODO: Should rename struct now that this was added.
    int leftmost_block_pos;
};

// Represents something a player can do with a block. Hold the block, or place it at a certain position with a certain rotation.
class Placement {

    // NOTE: Getters represent immutability without implicitly deleting ruining move/copy operations.

public:

    Placement(int _rotation, int _column, bool _is_hold)
        : rotation{_rotation}, column{_column}, is_hold{_is_hold}{
    }

    int get_rotation() const {
        return rotation;
    }

    int get_column() const {
        return column;
    }

    bool get_is_hold() const {
        return is_hold;
    }

private:
    int rotation;
    int column;
    bool is_hold;
};

// Exactly one instance for every type of block (tetrimino)
struct Block {

    std::string name;

    // maps[rotation idx] = maps for that rotation
    // size = number of possible rotations.
    std::vector<CH_maps> maps;

    // After this many translations, the piece will be against the wall (while in rotation 0)
    int safe_left_trans;
    int safe_right_trans;

    static const Block Blue;
    static const Block Purple;
    static const Block Red;
    static const Block Cyan;
    static const Block Yellow;
    static const Block Orange;
    static const Block Green;

    static const Block* char_to_block_ptr(char c);
    static const char block_ptr_to_char(const Block* block);

    int get_max_valid_placement_col(int rot_x) const;

    Block& operator=(const Block& other) = delete;
    Block& operator=(Block&& other) = delete;

    Block(const Block& other) = delete;
    Block(Block&& other) = delete;

private:
    Block(const std::string& _name, const std::vector<CH_maps>& _maps);
};

class Block_generator{

public:

    virtual const Block* generate() = 0;
    virtual ~Block_generator() = default;
};


class Stdin_block_generator : public Block_generator {

public:

    const Block* generate() override;
};

class Random_block_generator : public Block_generator {

public:

    using Seed_t = unsigned int;

    Random_block_generator(Seed_t seed);
    const Block* generate() override;

private:

    static std::vector<const Block*> full_bag;

    std::queue<const Block*> bag_instance;
    std::default_random_engine generator;
};


#endif