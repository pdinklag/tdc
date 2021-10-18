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
    static const char* ARG_OBJNAME;
    static const char* ARG_FREE;

    static constexpr size_t NIL = SIZE_MAX;

    static bool match_signature(const nlohmann::json& signature, const nlohmann::json& config);

    using ExecutableConstructor = std::function<std::unique_ptr<Executable>()>;

    struct RegisteredExecutable {
        AlgorithmInfo         info;
        nlohmann::json        signature;
        ExecutableConstructor construct;
    };

    template<typename E>
    requires std::derived_from<E, Executable>
    RegisteredExecutable make_registry_entry() {
        auto info = E::info();
        auto sig = info.signature();
        return { std::move(info), std::move(sig), [](){ return std::make_unique<E>(); } };
    }

    std::vector<RegisteredExecutable> registry_;
    RegisteredExecutable default_;

    nlohmann::json parse_cmdline(int argc, char** argv) const;
    void configure(const Config& cfg);

public:
    Application() : default_( { AlgorithmInfo(), nlohmann::json::object(), [](){ return std::unique_ptr<Executable>(); } }) {
    }

    template<typename E>
    requires std::derived_from<E, Executable> && Algorithm<E>
    void default_executable() {
        default_ = make_registry_entry<E>();
    }

    template<typename E>
    requires std::derived_from<E, Executable> && Algorithm<E>
    void register_executable() {
        registry_.emplace_back(make_registry_entry<E>());
    }

    inline void register_executables(tl::empty) {
        // nothing to do
    }

    template<Algorithm E, Algorithm... Es>
    inline void register_executables(tl::list<E, Es...>) {
        register_executable<E>();
        register_executables(tl::list<Es...>());
    }

    int run(int argc, char** argv) const;
};

}
