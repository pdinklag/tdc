#include <tdc/io/load_file.hpp>
#include <tdc/uint/uint40.hpp>

template std::vector<uint8_t> tdc::io::load_file_as_vector<uint8_t>(const std::filesystem::path&, const size_t);
template std::vector<uint16_t> tdc::io::load_file_as_vector<uint16_t>(const std::filesystem::path&, const size_t);
template std::vector<uint32_t> tdc::io::load_file_as_vector<uint32_t>(const std::filesystem::path&, const size_t);
template std::vector<tdc::uint40_t> tdc::io::load_file_as_vector<tdc::uint40_t>(const std::filesystem::path&, const size_t);
template std::vector<uint64_t> tdc::io::load_file_as_vector<uint64_t>(const std::filesystem::path&, const size_t);

template std::vector<uint8_t> tdc::io::load_file_lines_as_vector<uint8_t>(const std::filesystem::path&);
template std::vector<uint16_t> tdc::io::load_file_lines_as_vector<uint16_t>(const std::filesystem::path&);
template std::vector<uint32_t> tdc::io::load_file_lines_as_vector<uint32_t>(const std::filesystem::path&);
template std::vector<tdc::uint40_t> tdc::io::load_file_lines_as_vector<tdc::uint40_t>(const std::filesystem::path&);
template std::vector<uint64_t> tdc::io::load_file_lines_as_vector<uint64_t>(const std::filesystem::path&);
