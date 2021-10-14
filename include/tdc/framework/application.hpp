#pragma once

#include <cstddef>

#include <iomanip> // DEBUG
#include <iostream> // DEBUG

#include <nlohmann/json.hpp>

#include <tdc/framework/executable.hpp>

namespace tdc::framework {

class Application {
private:
    static constexpr char SYM_ASSIGN = '=';
    static constexpr char SYM_DEREF  = '.';

    static constexpr size_t NIL = SIZE_MAX;

    static const char* ARG_OBJNAME;
    static const char* ARG_FREE;

    static nlohmann::json cmdline_to_json(int argc, char** argv);

    Executable* exe_;

public:
    Application(Executable& exe) : exe_(&exe) {
    }

    void run(int argc, char** argv) {
        // parse command line into json
        auto config = cmdline_to_json(argc, argv);
        std::cout << std::setw(4) << config << std::endl;

        // TODO: determine signature and instantiate executable

        // TODO: configure executable
        
        // TODO: construct input and output
        int in, out;

        // run executable
        exe_->execute(in, out);
    }
};

}
