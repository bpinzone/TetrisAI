#ifndef HUMAN_ACTION_H
#define HUMAN_ACTION_H

struct Block;
struct Placement;

#include <iosfwd>

class HumanAction {

public:

    HumanAction(const Block* block, Placement placement);

    void print_to_stdout() const;

private:

    int offset;
    int rotation_count;
    bool hold;
};


#endif