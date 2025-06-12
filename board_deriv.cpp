#include "board_deriv.h"


Board_deriv::Board_deriv(){
    reset();
}

void Board_deriv::reset(){
    num_trenches = 0;
    at_least_one_side_clear = true;
    lowest_height = 0;
    second_lowest_height = 0;
    highest_height = 0;
    sum_of_squared_heights = 0;
    is_tetrisable = false;
}

void Board_deriv::update(const Board_foundation& foundation){
    static constexpr int impossibly_high_wall = BoardSize::c_rows + 5;
    static constexpr int min_depth_considered_trench = 3;

    num_trenches = 0;
    at_least_one_side_clear = (foundation.height_map[0] == 0) || (foundation.height_map[BoardSize::c_cols - 1] == 0);
    lowest_height = BoardSize::c_rows;
    second_lowest_height = BoardSize::c_rows;
    highest_height = 0;
    sum_of_squared_heights = 0;

    int left_height = impossibly_high_wall;
    int middle_height = foundation.height_map[0];
    int right_height = foundation.height_map[1];

    int some_trench_height = 0;

    for(int col_x = 0; col_x < BoardSize::c_cols; ++col_x){

        sum_of_squared_heights += middle_height * middle_height;
        second_lowest_height = middle_height <= lowest_height ? lowest_height : std::min(second_lowest_height, middle_height);
        lowest_height = std::min(lowest_height, middle_height);
        highest_height = std::max(highest_height, middle_height);

        // count and keep track of a trench.
        if(left_height - middle_height >= min_depth_considered_trench
                && right_height - middle_height >= min_depth_considered_trench){
            ++num_trenches;
            some_trench_height = middle_height;
        }

        left_height = middle_height;
        middle_height = right_height;
        right_height = (col_x == BoardSize::c_cols - 2) ? impossibly_high_wall : foundation.height_map[col_x + 2];
    }

    is_tetrisable =
        num_trenches == 1
        && lowest_height == some_trench_height
        && second_lowest_height >= some_trench_height + 4;
}
