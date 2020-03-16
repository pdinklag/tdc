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
/// \tparam out_vector_t the output vector type
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
            f.read((char*)buffer, num * CHAR_BIT * sizeof(in_t));

            for(size_t i = 0; i < num; i++) {
                v[i++] = (out_t)buffer[i];
            }

            left -= num;
        }
    }
    
    delete[] buffer;
    return v;
}

}} // namespace tdc::io
