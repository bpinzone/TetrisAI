#ifndef GLOBAL_STATS_H
#define GLOBAL_STATS_H

extern long gs_num_comparisons;

size_t get_num_leaves();
void incr_num_leaves();
void set_num_leaves(size_t num);


#endif