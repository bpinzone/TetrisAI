#include "global_stats.h"
#include <mutex>

long gs_num_comparisons = 0;

static size_t num_leaves = 0;
static std::mutex num_leaves_mutex;

size_t get_num_leaves() {
    return num_leaves;
}
void incr_num_leaves(){
    std::unique_lock<std::mutex> num_leaves_ulock(num_leaves_mutex);
    ++num_leaves;
}

void set_num_leaves(size_t num){
    std::unique_lock<std::mutex> num_leaves_ulock(num_leaves_mutex);
    num_leaves = num;
}