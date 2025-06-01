#ifndef UTILITY_H
#define UTILITY_H

#include <iostream>
#include <fstream>

// Reads a string from the stream, and throws an exception if the string is not what is expected.
void read_string_or_throw(std::istream& is, const std::string& expected);

// Coordinate where commands and board info a streamed to.
class Output_manager {

public:

    static Output_manager& get_instance(){
        static Output_manager om;
        return om;
    }

    // Call exactly once.
    void set_streams(char mode);

    // not available in tournament mode.
    std::ostream& get_command_os() const;

    // not available in tournament mode.
    std::ostream& get_board_os() const;

    std::ostream& get_ui_os() const;

    std::ostream& get_log_os() const;

    Output_manager(const Output_manager& other) = delete;
    Output_manager(Output_manager&& other) = delete;

    Output_manager& operator=(const Output_manager& other) = delete;
    Output_manager& operator=(Output_manager&& other) = delete;

    ~Output_manager();

private:

    Output_manager(){}

    char mode;
    std::ostream* command_os = nullptr;
    std::ostream* board_os = nullptr;
    std::ostream* ui_os = nullptr;
    std::ostream* log_os = nullptr;
};

#endif