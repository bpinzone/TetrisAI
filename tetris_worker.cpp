#include "tetris_worker.h"

#include <utility>
#include <functional>
#include <cassert>
#include <cmath>
#include <algorithm>

using std::vector;
using std::unique_lock;
using std::thread;
using std::mutex;
using std::condition_variable;
using std::move;
using std::bind;
using std::ceil;
using std::max_element;

static const int c_num_to_consider_with_head_down = 5000;
static const int c_offload_threshold = 1000000;

Tetris_worker::Tetris_worker(){
    t = thread{bind(&Tetris_worker::run, this)};
}

void Tetris_worker::restart_with_stack(vector<State>&& _state_stack){
    assert(!_state_stack.empty());
    // best_state = State::generate_worst_state_from(_state_stack.front());
    best_state = {};
    lock_set_signal_stack(move(_state_stack));
}

void Tetris_worker::lock_set_signal_stack(vector<State>&& _state_stack){
    unique_lock<mutex> ss_ulock(ss_mutex);
    state_stack = move(_state_stack);
    ss_ulock.unlock();
    ss_empty.notify_all();
}

void Tetris_worker::create_workers(int num_workers){
    for(int i = 0; i < num_workers; ++i){
        // Destroyed on program exit. Workers is static.
        // Assumes we never want to destroy workers until program ends.
        workers.push_back(new Tetris_worker);
    }
}

void Tetris_worker::wait_until_all_free(){
    unique_lock<mutex> fw_ulock(free_workers_mutex);
    free_worker_added.wait(fw_ulock, [](){
        return free_workers.size() == workers.size();
    });
}

// TODO: comment out calls to this later. Dont' want to lock so much.
void Tetris_worker::assert_all_free(){

    unique_lock<mutex> fw_ulock(free_workers_mutex);
    assert(workers.size() == free_workers.size());
}

void Tetris_worker::distribute_new_work_and_wait_till_all_free(State&& root_state){

    assert_all_free();

    unique_lock<mutex> fw_ulock(free_workers_mutex);
    for(auto& fw : free_workers){
        unique_lock<mutex> fw_ulock(fw->ss_mutex);
        assert(fw->state_stack.empty());
        fw->best_state = {};
    }

    // Make first generation.
    vector<State> first_gen;
    for(auto op_child = root_state.generate_next_child();
            op_child; op_child = root_state.generate_next_child()){
        first_gen.push_back(move(*op_child));
    }

    // Compute how many states each worker receives
    const size_t states_per_worker = static_cast<size_t>(
        ceil(static_cast<double>(first_gen.size()) / workers.size())
    );

    // Hand out work.
    vector<State> work_chunk;
    // int worker_x = 0;
    for(auto& state : first_gen){
        work_chunk.push_back(move(state));
        // Give out chunk
        if(work_chunk.size() == states_per_worker){
            free_workers.back()->restart_with_stack(move(work_chunk));
            free_workers.pop_back();
            work_chunk = decltype(work_chunk){};
        }
    }
    if(!work_chunk.empty()){
        free_workers.back()->restart_with_stack(move(work_chunk));
        free_workers.pop_back();
    }

    free_worker_added.wait(fw_ulock, [](){
        return free_workers.size() == workers.size();
    });
    fw_ulock.unlock();

}

State& Tetris_worker::get_best_reachable_state(){
    assert_all_free();
    Tetris_worker* best_worker = *max_element(free_workers.begin(), free_workers.end(),
        [](const auto& fw1, const auto& fw2){
            // Necessary because now a worker may not have any results to contribute.
            bool fw1_empty = !fw1->best_state.has_value();
            bool fw2_empty = !fw2->best_state.has_value();
            if(fw1_empty != fw2_empty){
                return fw1_empty;
            }
            if(fw1_empty && fw2_empty){
                return false; // arbitrary.
            }
            return fw2->best_state->get_board().has_greater_utility_than(
                fw1->best_state->get_board()
            );
    });
    return *best_worker->best_state;
}

void Tetris_worker::run(){

    unique_lock<mutex> ss_ulock{ss_mutex};

    while(true){

        // We have our ss_ulock.
        // Wait for work if necessary.
        if(state_stack.empty()){
            // Need if statement because threads start with non-empty stacks.

            // Mark ourselves as free.
            unique_lock<mutex> free_workers_ulock{free_workers_mutex};
            free_workers.push_back(this);
            free_workers_ulock.unlock();
            // Tell master thread to check if we're all here.
            free_worker_added.notify_all();

            // What if we're interrupted here?
            // It is OK that the worker is marked free first, because someone should never signal us without first acquiring our ss_mutex.

            // Wait until something is put in our stack.
            // Predicate represents: Are we ready to go?
            ss_empty.wait(ss_ulock, [this](){return !state_stack.empty();});
        }
        // We have ss_ulock and state_stack is not empty
        assert(!state_stack.empty());

        int num_considered_with_head_down = 0;

        while(!state_stack.empty()){
            // We have work to do.

            // Do our work.
            State considered_state = move(state_stack.back());
            state_stack.pop_back();
            if(considered_state.get_is_leaf()){
                if(!best_state || considered_state.get_board().has_greater_utility_than(best_state->get_board())){
                    best_state = move(considered_state);
                }
            }
            else{
                for(auto op_child = considered_state.generate_next_child();
                        op_child; op_child = considered_state.generate_next_child()){
                    state_stack.push_back(move(*op_child));
                }
            }

            // Attempt to offload work periodically if appropriate.
            ++num_considered_with_head_down;
            if(num_considered_with_head_down >= c_num_to_consider_with_head_down
                    && state_stack.size() >= c_offload_threshold){
                num_considered_with_head_down = 0;
                attempt_to_offload_work();
            }
        } // not empty

        // Done with our work.

    } // true
} // run

void Tetris_worker::attempt_to_offload_work(){

    Tetris_worker* free_worker = nullptr;
    unique_lock<mutex> free_workers_ulock(free_workers_mutex);
    if(!free_workers.empty()){
        free_worker = free_workers.back();
        free_workers.pop_back();
    }
    free_workers_ulock.unlock();

    if(!free_worker){
        return;
    }

    // Offload
    vector<State> my_new_state_stack;
    vector<State> fw_new_state_stack;

    bool to_me = true;
    for(auto& state : state_stack){
        if(to_me){
            my_new_state_stack.push_back(move(state));
        }
        else{
            fw_new_state_stack.push_back(move(state));
        }
        to_me = !to_me;
    }
    state_stack = move(my_new_state_stack);
    free_worker->lock_set_signal_stack(move(fw_new_state_stack));
}