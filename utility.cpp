#include "utility.h"

#include <stdexcept>

using std::runtime_error;
using std::ofstream;
using std::endl;
using std::cout;

void Output_manager::set_streams(char mode){
    static bool set = false;
    if(!set){
        set = true;
    }
    else{
        // Should really be an assertion.
        throw runtime_error{"Cannot set streams more than once!"};
    }
    if(mode == 'w'){
        command_os = new ofstream("commands.log");
        board_os = &cout;
        // Truncate unused log file.
        ofstream fout{"board.log"};
        fout << endl;
        fout.close();
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
}