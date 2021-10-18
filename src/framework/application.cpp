#include <tdc/framework/application.hpp>

#include <cstring>

namespace tdc::framework {

const char* Application::ARG_FREE    = "__args";
const char* Application::ARG_OBJNAME = "__name";

nlohmann::json Application::parse_cmdline(int argc, char** argv) const {
    // prepare
    auto obj = default_.signature;

    // walk arguments, skip first
    for(int i = 1; i < argc; i++) {
        char* arg = argv[i];
        size_t arglen = std::strlen(arg);

        if(arglen > 0) {
            if(arg[0] == '-') {
                if(arglen >= 2 && arg[1] == '-') {
                    // long parameter name
                    arg += 2;
                    arglen -= 2;
                } else {
                    // short parameter name
                    // TODO: mapping?
                    arg += 1;
                    arglen -= 1;
                }

                // parse argument and create object path
                // stop at "equals" symbol
                nlohmann::json* current_obj = &obj;
                char* current_arg = arg;
                char* value = nullptr;
                for(size_t j = 0; j < arglen; j++) {
                    if(arg[j] == SYM_ASSIGN) {
                        // we found the assignment operator - stop parsing
                        arg[j] = 0;
                        value = arg + j + 1;
                        break;
                    } else if(arg[j] == SYM_DEREF) {
                        // we found a dereferencing operator - create new subobject of current object
                        arg[j] = 0;
                        if(current_obj->contains(current_arg)) {
                            auto& v = (*current_obj)[current_arg];
                            if(v.is_object()) {
                                current_obj = &v;
                            } else {
                                nlohmann::json new_obj;
                                new_obj[ARG_OBJNAME] = v;

                                (*current_obj)[current_arg] = new_obj;
                                current_obj = &(*current_obj)[current_arg];
                            }
                        } else {
                            (*current_obj)[current_arg] = nlohmann::json::object();
                            current_obj = &(*current_obj)[current_arg];
                        }

                        current_arg = arg + j + 1;
                    }
                }

                // assign
                if(value) {
                    if(current_obj->contains(current_arg)) {
                        auto& v = (*current_obj)[current_arg];
                        if(v.is_object()) {
                            v[ARG_OBJNAME] = value;
                        } else if(v.is_array()) {
                            v.push_back(value);
                        } else {
                            (*current_obj)[current_arg] = { v, value };
                        }
                    } else {
                        (*current_obj)[current_arg] = value;
                    }
                } else {
                    (*current_obj)[current_arg] = true;
                }
            } else {
                // this is a free argument
                obj[ARG_FREE].push_back(arg);
            }
        }
    }

    // done
    return obj;
}

bool Application::match_signature(const nlohmann::json& signature, const nlohmann::json& config) {
    // try to match each item in the signature
    for(const auto& p : signature.items()) {
        // make sure subject contains this key
        if(!config.contains(p.key())) {
            return false;
        }

        // attempt to match the value
        const auto& pv = p.value();
        const auto& cv = config[p.key()];

        if(pv.is_object() && pv.contains(ARG_OBJNAME)) {
            // we are trying to match a sub-algorithm entry
            if(pv.size() == 1) {
                const auto& pname = pv[ARG_OBJNAME];

                // there is no further nesting, so in the config we expect either an object containing the correct name, or a string that represents this name
                if(cv.is_object()) {
                    // the config must either contain an object containing the correct name...
                    if(!cv.contains(ARG_OBJNAME) || cv[ARG_OBJNAME] != pname) {
                        return false;
                    }
                } else if(cv.is_string()) {
                    // ... or it can be just the object name as a string
                    if(cv != pname) {
                        return false;
                    }
                } else {
                    // it's not even an object
                    return false;
                }
            } else {
                // there is further nesting - recurse
                if(!match_signature(pv, cv)) {
                    // recursive match failed
                    return false;
                }
            }
        } else if(pv != cv) {
            // primitive value match failed
            return false;
        }
    }

    // all elements have successfully matched
    return true;
}

int Application::run(int argc, char** argv) const {
    // parse command line into json
    auto config = parse_cmdline(argc, argv);

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

}
