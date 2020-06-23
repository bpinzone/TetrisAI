#ifndef UTILITY_H
#define UTILITY_H

#include <iostream>
#include <fstream>

class Output_manager {

public:

    static Output_manager& get_instance(){
        static Output_manager om;
        return om;
    }

    // Call exactly once.
    void set_streams(char mode);

    std::ostream& get_command_os() const {
        return *command_os;
    }

    std::ostream& get_board_os() const {
        return *board_os;
    }

    Output_manager(const Output_manager& other) = delete;
    Output_manager(Output_manager&& other) = delete;

    Output_manager& operator=(const Output_manager& other) = delete;
    Output_manager& operator=(Output_manager&& other) = delete;

    ~Output_manager();
private:

    Output_manager(){}

    std::ostream* command_os = nullptr;
    std::ostream* board_os = nullptr;
};

#endif