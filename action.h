#ifndef ACTION_H
#define ACTION_H

struct Block;
struct Placement;

#include <iosfwd>

class Action {

public:
    Action(const Block* block, Placement placement);
    friend std::ostream& operator<<(std::ostream& os, const Action& action);

private:
    int offset;
    int rotation_count;
    bool hold;
};


#endif