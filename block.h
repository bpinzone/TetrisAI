#ifndef BLOCK_H
#define BLOCK_H

#include <string>
#include <vector>
#include <cassert>
#include <random>

struct CH_maps {
    std::vector<int> contour;
    std::vector<int> height;
    int leftmost_block_pos;
};

class Placement {

public:

    Placement(int _rotation, int _column, bool _is_hold)
        : rotation{_rotation}, column{_column}, is_hold{_is_hold}{
    }

    int get_rotation() const {
        assert(!is_hold);
        return rotation;
    }

    int get_column() const {
        assert(!is_hold);
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

    // [rotation] = maps for that rotation
    std::string name;
    std::vector<CH_maps> maps;

    static const Block Blue;
    static const Block Purple;
    static const Block Red;
    static const Block Cyan;
    static const Block Yellow;
    static const Block Orange;
    static const Block Green;

    int get_max_valid_placement_col(int rot_x) const;

    Block& operator=(const Block& other) = delete;
    Block& operator=(Block&& other) = delete;

    Block(const Block& other) = delete;
    Block(Block&& other) = delete;

private:
    Block(const std::string& _name, const std::vector<CH_maps>& _maps);
};


class Block_generator {

public:

    using Seed_t = unsigned int;

    Block_generator(Seed_t seed);

    const Block* operator()();

private:

    static const Block* block_ptrs[];

    static std::uniform_int_distribution<int> distribution;

    std::default_random_engine generator;
};


#endif