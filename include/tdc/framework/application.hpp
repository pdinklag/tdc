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

    static nlohmann::json parse_cmdline(int argc, char** argv);
    static bool match_signature(const nlohmann::json& signature, const nlohmann::json& config);

    Executable* exe_;

public:
    Application(Executable& exe) : exe_(&exe) {
    }

    void run(int argc, char** argv) {
        // parse command line into json
        auto config = parse_cmdline(argc, argv);
        std::cout << std::setw(4) << config << std::endl;

        auto demo = R"({
            "code": {
                "__name": "BidirectionalLZ",
                "refCode": {
                    "__name": "Binary"
                },
                "lenCode": {
                    "__name": "Rice"
                },
                "charCode": {
                    "__name": "Huffman"
                }
            },
            "strategy": {
                "__name": "Arrays"
            }
        })"_json;

        // TODO: find matching signature and instantiate executable
        std::cout << std::setw(4) << demo << std::endl;

        std::cout << "match: " << match_signature(demo, config) << std::endl;

        // TODO: configure executable
        
        // TODO: construct input and output
        int in, out;

        // run executable
        exe_->execute(in, out);
    }
};

}
