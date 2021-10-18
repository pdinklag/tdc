#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace tdc::framework {

/**
 * @brief Represents a configuration of an @ref Algorithm, typically created by an @ref Application after parsing the command line.
 * 
 */
class Config {
private:
    const nlohmann::json* json_;

public:
    /**
     * @brief Constructs a configuration from a JSON dataset.
     * 
     * @param json the JSON dataset
     */
    inline Config(const nlohmann::json& json) : json_(&json) {
    }

    /**
     * @brief Gets a sub configuration.
     * 
     * @param param the parameter name
     * @return the sub configuration, or an empty configuration if the specified parameter does not exist
     */
    Config sub_config(const std::string& param) const;

    /**
     * @brief Gets the boolean value of a parameter.
     * 
     * @param param the parameter name
     * @param out a reference to the variable into which to store the result
     * @return @c true if a value has been read successfully and stored in @c out
     * @return @c false if no value has been read
     */
    bool get_flag(const std::string& param, bool& out) const;

    /**
     * @brief Gets the integer value of a parameter.
     * 
     * @param param the parameter name
     * @param out a reference to the variable into which to store the result
     * @return @c true if a value has been read successfully and stored in @c out
     * @return @c false if no value has been read
     */
    bool get_int(const std::string& param, int& out) const;

    /**
     * @brief Gets the unsigned integer value of a parameter.
     * 
     * @param param the parameter name
     * @param out a reference to the variable into which to store the result
     * @return @c true if a value has been read successfully and stored in @c out
     * @return @c false if no value has been read
     */
    bool get_uint(const std::string& param, unsigned long& out) const;

    /**
     * @brief Gets the size value of a parameter.
     * 
     * @param param the parameter name
     * @param out a reference to the variable into which to store the result
     * @return @c true if a value has been read successfully and stored in @c out
     * @return @c false if no value has been read
     */
    bool get_size_t(const std::string& param, size_t& out) const;

    /**
     * @brief Gets the single-precision floating point value of a parameter.
     * 
     * @param param the parameter name
     * @param out a reference to the variable into which to store the result
     * @return @c true if a value has been read successfully and stored in @c out
     * @return @c false if no value has been read
     */
    bool get_float(const std::string& param, float& out) const;

    /**
     * @brief Gets the double-precision floating point value of a parameter.
     * 
     * @param param the parameter name
     * @param out a reference to the variable into which to store the result
     * @return @c true if a value has been read successfully and stored in @c out
     * @return @c false if no value has been read
     */
    bool get_double(const std::string& param, double& out) const;

    /**
     * @brief Gets the string point value of a parameter.
     * 
     * @param param the parameter name
     * @param out a reference to the variable into which to store the result
     * @return @c true if a value has been read successfully and stored in @c out
     * @return @c false if no value has been read
     */
    bool get_string(const std::string& param, std::string& out) const;

    /**
     * @brief Gets the string list value of a parameter.
     * 
     * @param param the parameter name
     * @param out a reference to the variable into which to store the result
     * @return @c true if a value has been read successfully and stored in @c out
     * @return @c false if no value has been read
     */
    bool get_stringlist(const std::string& param, std::vector<std::string>& out) const;

    /**
     * @brief Gets the number of bytes value of a parameter.
     * 
     * This accepts values stated with SI units, e.g., @c 100K for 100 kilobytes, or @c 10Mi for 10 mebibytes.
     * 
     * @param param the parameter name
     * @param out a reference to the variable into which to store the result
     * @return @c true if a value has been read successfully and stored in @c out
     * @return @c false if no value has been read
     */
    bool get_bytes(const std::string& param, uint64_t& out) const;

    /**
     * @brief Gets the boolean value of a parameter.
     * 
     * This is an alias for @ref get_flag.
     * 
     * @param param the parameter name
     * @param out a reference to the variable into which to store the result
     * @return @c true if a value has been read successfully and stored in @c out
     * @return @c false if no value has been read
     */
    inline bool get_bool(const std::string& param, bool& out) const {
        return get_flag(param, out);
    }
};

}
