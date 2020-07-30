#ifndef BLOCK_H
#define BLOCK_H

#include <string>
#include <vector>
#include <queue>
#include <cassert>
#include <random>

struct CH_maps {
    std::vector<int> contour;
    std::vector<int> height;
    // TODO: Should rename struct now that this was added.
    int leftmost_block_pos;
};

class Placement {

    // NOTE: Getters represent immutability without implicitly deleting ruining move/copy operators.

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

struct Block {

    std::string name;

    // maps[rotation] = maps for that rotation
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