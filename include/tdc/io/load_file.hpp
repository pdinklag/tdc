#pragma once

#include <tdc/util/literals.hpp>

#include <climits>
#include <filesystem>
#include <fstream>
#include <vector>

namespace tdc {
namespace io {

/// \brief Loads a binary file into an integer vector.
/// \tparam in_t the input type, which determines the number of bytes read for each integer
/// \tparam out_t the output type
/// \tparam out_vector_t the output vector type, must support a constructor accepting the size
/// \param path the path to the file to read
/// \param bufsize the read buffer size
template<typename in_t, typename out_t = in_t, typename out_vector_t = std::vector<out_t>>
inline out_vector_t load_file_as_vector(
    const std::filesystem::path& path,
    const size_t bufsize = 1_Mi) {

    const size_t sz = std::filesystem::file_size(path);
    const size_t num_items = sz / sizeof(in_t);
    
    out_vector_t v(num_items);
    in_t* buffer = new in_t[bufsize];

    {
        std::ifstream f(path);
        size_t i = 0;
        size_t left = num_items;
        while(left) {
            const size_t num = std::min(bufsize, left);
            f.read((char*)buffer, num * sizeof(in_t));

            for(size_t i = 0; i < num; i++) {
                v[i++] = (out_t)buffer[i];
            }

            left -= num;
        }
    }
    
    delete[] buffer;
    return v;
}

/// \brief Loads a text file into an integer vector by parsing each line as an integer value.
/// \tparam out_t the output type
/// \tparam out_vector_t the output vector type, must support \c push_back
/// \param path the path to the file to read
template<typename out_t, typename out_vector_t = std::vector<out_t>>
inline out_vector_t load_file_lines_as_vector(const std::filesystem::path& path) {
    out_vector_t v;
    std::ifstream f(path);

    // 2^64 has 20 decimal digits, so a buffer of size 24 should be safe
    for(std::array<char, 24> linebuf; f.getline(&linebuf[0], 24);) {
        if(linebuf[0]) {
            v.push_back((out_t)uint64_t(std::atoll(&linebuf[0])));
        }
    }
    return v;
}

}} // namespace tdc::io
