#ifndef PLAY_SETTINGS_H
#define PLAY_SETTINGS_H

class Block_generator;

// ALL gathered from command line.
struct Play_settings {

    /*
    w: watch jeff play in terminal
    s: 99 skip
    m: 99 from menu
    r: 99 from restart
    */
    char mode;

    /*
    <seed int> OR i (stands for input)
    */
    Block_generator* block_generator;

    // [1, 7]
    int lookahead_placements;

    // [0, 6] (check this)
    int queue_size;

    int game_length;

    int num_threads;

    // NOTE: IMPORTANT
    inline static constexpr int num_settings = 6;

    Play_settings(int argc, char* argv[]);

    bool is_watching() const {
        return mode == 'w';
    }

    void wait_for_controller_connection_if_necessary();
};

#endif