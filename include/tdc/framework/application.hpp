#pragma once

#include <concepts>
#include <cstddef>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include <iomanip> // DEBUG
#include <iostream> // DEBUG

#include <nlohmann/json.hpp>

#include <tdc/framework/executable.hpp>
#include <tdc/util/type_list.hpp>

namespace tdc::framework {

class Application {
private:
    static constexpr char SYM_ASSIGN = '=';
    static constexpr char SYM_DEREF  = '.';

    static const char* ARG_OBJNAME;
    static const char* ARG_FREE;

    static constexpr size_t NIL = SIZE_MAX;

    static nlohmann::json parse_cmdline(int argc, char** argv);
    static bool match_signature(const nlohmann::json& signature, const nlohmann::json& config);

    using ExecutableConstructor = std::function<std::unique_ptr<Executable>()>;

    struct RegisteredExecutable {
        AlgorithmInfo         info;
        nlohmann::json        signature;
        ExecutableConstructor construct;
    };

    std::string root_name_;
    std::vector<RegisteredExecutable> registry_;

public:
    Application(std::string&& root_name) : root_name_(std::move(root_name)) {
    }

    template<typename E>
    requires std::derived_from<E, Executable> && Algorithm<E>
    void register_executable() {
        auto info = E::info();
        auto sig = info.signature();

        std::cout << "[" << registry_.size() << "] = " << sig << std::endl;
        registry_.emplace_back(std::move(info), std::move(sig), [](){ return std::make_unique<E>(); });
    }

    inline void register_executables(tl::empty) {
        // nothing to do
    }

    template<Algorithm E, Algorithm... Es>
    inline void register_executables(tl::list<E, Es...>) {
        register_executable<E>();
        register_executables(tl::list<Es...>());
    }

    int run(int argc, char** argv) {
        // parse command line into json
        auto config = parse_cmdline(argc, argv);
        if(!config.contains(ARG_OBJNAME)) {
            config[ARG_OBJNAME] = root_name_;
        }

        // find a matching configuration and instantiate executable
        std::unique_ptr<Executable> exe;
        for(auto& r : registry_) {
            if(match_signature(r.signature, config)) {
                std::cout << "found matching configuration: " << r.signature << std::endl;
                exe = r.construct();
                break;
            }
        }

        if(!exe) {
            std::cerr << "failed to find a matching configuration: " << config << std::endl;
            return -1;
        }

        // TODO: configure executable
        
        // TODO: construct input and output
        int in, out;

        // TODO: run executable
        return exe->execute(in, out);
    }
};

}
