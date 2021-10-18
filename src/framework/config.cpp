#include <stdexcept>

#include <tdc/framework/config.hpp>
#include <tlx/string/parse_si_iec_units.hpp>

nlohmann::json empty_json = nlohmann::json::object();

bool is_true(const std::string& s) {
    return s == "1" || s == "true";
}

template<typename T, typename Parser>
bool get_number(const nlohmann::json* json_, const std::string& param, T& out, Parser parse) {
    if(json_->contains(param)) {
        const auto& v = (*json_)[param];
        if(v.is_string()) {
            try {
                out = parse(v.get<std::string>());
                return true;
            } catch(const std::invalid_argument&) {
            } catch(const std::out_of_range&) {
            }
        } else if(v.is_number()) {
            out = v.get<T>();
            return true;
        }
    }
    return false;
}

namespace tdc::framework {
    Config Config::sub_config(const std::string& param) const {
        return Config(json_->contains(param) ? (*json_)[param] : empty_json);
    }

    bool Config::get_flag(const std::string& param, bool& out) const {
        if(json_->contains(param)) {
            const auto& v = (*json_)[param];
            if(v.is_boolean()) {
                out = v.get<bool>();
                return true;
            } else if(v.is_string()) {
                out = is_true(v.get<std::string>());
                return true;
            }
        }
        return false;
    }

    bool Config::get_int(const std::string& param, int& out) const {
        return ::get_number<int>(json_, param, out, [](const std::string& s){ return std::stoi(s); });
    }

    bool Config::get_uint(const std::string& param, unsigned long& out) const {
        return ::get_number<unsigned long>(json_, param, out, [](const std::string& s){ return std::stoul(s); });
    }

    bool Config::get_size_t(const std::string& param, size_t& out) const {
        return ::get_number<size_t>(json_, param, out, [](const std::string& s){ return (size_t)std::stoull(s); });
    }

    bool Config::get_float(const std::string& param, float& out) const {
        return ::get_number<float>(json_, param, out, [](const std::string& s){ return std::stof(s); });
    }

    bool Config::get_double(const std::string& param, double& out) const {
        return ::get_number<double>(json_, param, out, [](const std::string& s){ return std::stod(s); });
    }

    bool Config::get_string(const std::string& param, std::string& out) const {
        if(json_->contains(param)) {
            const auto& v = (*json_)[param];
            if(v.is_string()) {
                out = v.get<std::string>();
                return true;
            }
        }
        return false;
    }

    bool Config::get_stringlist(const std::string& param, std::vector<std::string>& out) const {
        if(json_->contains(param)) {
            std::vector<std::string> list;

            const auto& a = (*json_)[param];
            if(a.is_array()) {
                list.reserve(a.size());
                for(const auto& e : a.items()) {                    
                    const auto& v = e.value();
                    if(v.is_string()) {
                        list.push_back(v.get<std::string>());
                    } else {
                        // not a string, abort
                        return false;
                    }
                }

                out = std::move(list);
                return true;
            } else if(a.is_string()) {
                // a single string is interpreted as a list of size 1
                list.reserve(1);
                list.push_back(a.get<std::string>());
                out = std::move(list);
                return true;
            }
        }
        return false;
    }

    bool Config::get_bytes(const std::string& param, uint64_t& out) const {
        if(json_->contains(param)) {
            const auto& v = (*json_)[param];
            if(v.is_number()) {
                out = v.get<uint64_t>();
                return true;
            } else if(v.is_string()) {
                uint64_t parse_result;
                if(tlx::parse_si_iec_units(v.get<std::string>(), &parse_result)) {
                    out = parse_result;
                    return true;
                }
            }
        }
        return false;
    }
}
