#ifndef ACTION_H
#define ACTION_H

struct Block;
struct Placement;

#include <iosfwd>

// Effectively converts a Placement into button presses to be output to Jeff's hands.
class Action {

public:
    Action(const Block* _block, Placement placement);
    friend std::ostream& operator<<(std::ostream& os, const Action& action);

    inline static bool switch_to_attackers = true;

private:
    const Block* block;
    int offset;
    int rotation_count;
    bool hold;
};


#endif