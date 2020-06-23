#ifndef ACTION_H
#define ACTION_H

struct Block;
struct Placement;

#include <iosfwd>
#include <string>

class Action {

public:
    Action(const Block* block, Placement placement, const std::string& _wait_command_to_follow);
    friend std::ostream& operator<<(std::ostream& os, const Action& action);

private:
    int offset;
    int rotation_count;
    bool hold;
    std::string wait_command_to_follow;
};


#endif