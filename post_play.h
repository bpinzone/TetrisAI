#ifndef POST_PLAY_H
#define POST_PLAY_H

#include "board.h"
#include "state.h"
#include <iosfwd>

struct Game_state {

    using Tetris_queue_t = State::Tetris_queue_t;

    Game_state(std::istream& is);
    Game_state(
            const Block* _presented,
            const Tetris_queue_t& _queue,
            const Board& _board)
        : presented{_presented},
        queue{_queue},
        board{_board} {
    }

    const Block* presented;
    Tetris_queue_t queue;
    Board board;
};

struct Vision_state {

    Vision_state(std::istream& is);

    Game_state game_state;
    bool use_last_resulting_board;
};

struct Post_play_report{

    Post_play_report(
            const Block* const _presented,
            const Board& _board,
            bool _just_held_non_first,
            bool _just_cleared_line)
        : presented{_presented},
        board{_board},
        just_held_non_first{_just_held_non_first},
        just_cleared_line{_just_cleared_line}{
    }


    const Block* presented;
    Board board;
    bool just_held_non_first;
    bool just_cleared_line;
};


#endif