#include "utility.h"

#include <stdexcept>

using std::runtime_error;
using std::ofstream;
using std::endl;
using std::cout;

void Output_manager::set_streams(char mode){
    this->mode = mode;

    static bool set = false;
    if(!set){
        set = true;
    }
    else{
        // Should really be an assertion.
        throw std::logic_error{"Cannot set streams more than once!"};
    }

    if(mode == 'w'){
        command_os = new ofstream("commands.log");
        board_os = &cout;
        // Truncate unused log file.
        ofstream fout{"board.log"};
        fout << endl;
        fout.close();
    }
    else if(mode == 't'){
        ui_os = &cout;
        log_os = new ofstream("tetris_log.txt");
    }
    else{
        command_os = &cout;
        board_os = new ofstream("board.log");
        // Truncate unused log file.
        ofstream fout{"commands.log"};
        fout << endl;
        fout.close();
    }
}

std::ostream& Output_manager::get_command_os() const {
    if(mode == 't'){
        throw std::logic_error{"Cannot get command_os in tournament mode!"};
    }
    return *command_os;
}

std::ostream& Output_manager::get_board_os() const {
    if(mode == 't'){
        throw std::logic_error{"Cannot get board_os in tournament mode!"};
    }
    return *board_os;
}
std::ostream& Output_manager::get_ui_os() const {
    return *ui_os;
}

std::ostream& Output_manager::get_log_os() const {
    return *log_os;
}

Output_manager::~Output_manager(){
    if(ofstream* fout = dynamic_cast<ofstream*>(command_os)){
        fout->close();
        delete fout;
        command_os = nullptr;
    }
    if(std::ofstream* fout = dynamic_cast<ofstream*>(board_os)){
        fout->close();
        delete fout;
        board_os = nullptr;
    }
    if(std::ofstream* fout = dynamic_cast<ofstream*>(ui_os)){
        fout->close();
        delete fout;
        ui_os = nullptr;
    }
    if(std::ofstream* fout = dynamic_cast<ofstream*>(log_os)){
        fout->close();
        delete fout;
        log_os = nullptr;
    }
}