#include <tdc/framework/application.hpp>

#include <cstring>

namespace tdc::framework {

const char* Application::ARG_FREE    = "__args";
const char* Application::ARG_OBJNAME = "__name";

nlohmann::json Application::cmdline_to_json(int argc, char** argv) {
    // prepare
    nlohmann::json obj;

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

}
