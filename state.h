#ifndef STATE_H
#define STATE_H

#include "board.h"
#include "block.h"

#include <deque>
#include <optional>

class State {

public:

    using Tetris_queue_t = std::deque<const Block*>;

    State(
        const Board& _board,
        const Block* _presented_block,
        const Tetris_queue_t::const_iterator& _next_queue_it,
        bool _is_leaf,
        const Tetris_queue_t::const_iterator& _end_queue_it,
        int _placement_limit,
        std::optional<Placement> _placement_taken_from_root
    );

    static State generate_root_state(
        const Board& board,
        const Block& presented,
        const Tetris_queue_t& queue,
        const int num_placements_to_look_ahead
    );

    static State generate_worst_state_from(const State& other);

    bool get_is_leaf() const {
        return is_leaf;
    }

    const Board& get_board() const {
        return board;
    }

    Placement get_placement_taken_from_root() const {
        assert(placement_taken_from_root);
        return *placement_taken_from_root;
    }


    // Generate the next child. Returns empty optional when there are no more children.
    std::optional<State> generate_next_child();

    // Disable copy semantics. Never want to copy the placement generator.
    State& operator=(const State& other) = delete;
    State(const State& other) = delete;

    State& operator=(State&& other) = default;
    State(State&& other) = default;

private:

    class Placement_generator{

    public:
        Placement_generator(const Block* _presented);
        std::optional<Placement> operator()();

    private:
        const Block* presented;
        int rot_x;
        int col;
        bool exhausted;
    };

    std::optional<State> generate_child_from_placement(Placement placement);

    // Actual State Info
    Board board;
    const Block* presented_block;
    Tetris_queue_t::const_iterator next_queue_it;

    // Leaf Logic
    // If this is true, only board should ever be read.
    bool is_leaf;
    Tetris_queue_t::const_iterator end_queue_it;
    int placement_limit;

    // Construction Logic
    // Empty -> This is the root.
    std::optional<Placement> placement_taken_from_root;

    Placement_generator pg;
};

#endif