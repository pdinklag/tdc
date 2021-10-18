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

/**
 * @brief Configures and runs an @ref Executable.
 * 
 * Applications are designed as the entry point of a program.
 * It parses the command line into a JSON configuration.
 * The resulting @ref Config is used to select, configure and ultimately run the matching registered @ref Executable, if any.
 * 
 * The command line is parsed into a JSON dataset as follows:
 * each argument starting with <tt>-</tt> or <tt>--</tt> is considered a parameter.
 * If a dot ( <tt>.</tt> ) is encountered in a parameter name, this results in the creation of a sub object into which the parameter is stored.
 * If an object parameter is assigned a value and it also has sub parameters, said value will be considered the object's @em name.
 * Values are assigned to parameters by using the equals ( <tt>=</tt> ) symbol; these are not interpreted before algorithms are configured and are stored simply as strings.
 * If a parameter is stated without a value, it will simply be assigned @c true, as in a set flag.
 * If the same parameter is assigned a value multiple times, it will result in a list containing all values in their order of occurrence.
 * Arguments that are not parameters are considered @em free parameters, reserved for the framework to be interpreted, typically specifying input filenames.
 * 
 * As an example, the command line <tt>-x --obj=A --obj.a=100 --obj.b=str in1 in2</tt> will be parsed to the following JSON:
 * @code {.json}
 * {
 *     "__free": ["in1", "in2"],
 *     "obj": {
 *         "__name": "A",
 *         "a": "100",
 *         "b": "str"
 *     },
 *     "x": true
 * }
 * @endcode
 * 
 * The parsed JSON is compared with the signatures of all registered executables.
 * If an executable's signature matches, the corresponding @ref Executable type is instantiated using its default constructor, @ref Algorithm::configure "configured" and finally @ref Executable::execute "executed".
 */
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

    /**
     * @brief Sets the default @ref Executable for the application.
     * 
     * @tparam E the default @ref Executable type
     */
    template<typename E>
    requires std::derived_from<E, Executable> && Algorithm<E>
    void default_executable() {
        default_ = make_registry_entry<E>();
    }

    /**
     * @brief Registers an @ref Executable in the application.
     * 
     * @tparam E the @ref Executable type to be registered
     */
    template<typename E>
    requires std::derived_from<E, Executable> && Algorithm<E>
    void register_executable() {
        registry_.emplace_back(make_registry_entry<E>());
    }

    /**
     * @brief Does nothing.
     * 
     * This is the endpoint for the unfolding of the variadic @ref register_executables method at compile time and serves no further purpose.
     */
    inline void register_executables(tl::empty) {
        // nothing to do
    }

    /**
     * @brief Registers multiple executables given by a @ref tl::list "type list".
     * 
     * @tparam E the first algorithm
     * @tparam Es the remaining algorithms
     */
    template<typename E, typename... Es>
    inline void register_executables(tl::list<E, Es...>) {
        register_executable<E>();
        register_executables(tl::list<Es...>());
    }

    /**
     * @brief Runs the application with the given command line.
     * 
     * This method is typically called by a @c main method.
     * See @ref Application for details on how the command line is parsed and interpreted.
     * 
     * @param argc the number of provided arguments
     * @param argv the arguments
     * @return the return code of the application; zero indicates success
     */
    int run(int argc, char** argv) const;
};

}
