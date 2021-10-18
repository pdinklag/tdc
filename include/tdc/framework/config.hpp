#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace tdc::framework {

class Config {
private:
    const nlohmann::json* json_;

public:
    inline Config(const nlohmann::json& json) : json_(&json) {
    }

    Config sub_config(const std::string& param) const;

    bool get_flag(const std::string& param, bool& out) const;
    bool get_int(const std::string& param, int& out) const;
    bool get_uint(const std::string& param, unsigned long& out) const;
    bool get_size_t(const std::string& param, size_t& out) const;
    bool get_float(const std::string& param, float& out) const;
    bool get_double(const std::string& param, double& out) const;
    bool get_string(const std::string& param, std::string& out) const;
    bool get_stringlist(const std::string& param, std::vector<std::string>& out) const;
    bool get_bytes(const std::string& param, uint64_t& out) const;

    inline bool get_bool(const std::string& param, bool& out) const {
        return get_flag(param, out);
    }
};

}
