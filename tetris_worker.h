#ifndef TETRIS_WORKER_H
#define TETRIS_WORKER_H

#include <vector>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <optional>

#include "state.h"

// Wraps a thread object, and can find the best state given a stack of states to search through.
class Tetris_worker {

public:

    Tetris_worker();

    // Reset what is best, and lock and set state stack.
    void restart_with_stack(std::vector<State>&& _state_stack);

    void lock_set_signal_stack(std::vector<State>&& _state_stack);

    static void create_workers(int num_workers);
    static void wait_until_all_free();
    static void assert_all_free();

    // Gives work to all threads and marks them all as not free.
    static void distribute_new_work_and_wait_till_all_free(State&& root_state);

    // Requires: Workers are free and they just finished doing work.
    static State& get_best_reachable_state();

private:

    void run();

    void attempt_to_offload_work();

    // Adding: free workers add themselves.
    // Removing: busy workers remove free workers.
    inline static std::vector<Tetris_worker*> free_workers;

    inline static std::mutex free_workers_mutex;
    inline static std::condition_variable free_worker_added;

    inline static std::vector<Tetris_worker*> workers;

    // Construction Order matters.
    std::optional<State> best_state;
    std::vector<State> state_stack;

    std::thread t;
    std::mutex ss_mutex;
    std::condition_variable ss_empty;
};

#endif