#include <tdc/io/mmap_file.hpp>
#include <tdc/test/assert.hpp>

#include <fstream>
#include <filesystem>
#include <type_traits>

std::string filename = "numbers";
uint64_t numbers[] = { 1, 2, 3, 4, 5, 6, 7, 8 };

int main(int argc, char** argv) {
    // write numbers to file
    {
        std::ofstream file(filename);
        file.write((const char*)&numbers, sizeof(numbers));
    }
    
    // map file and check numbers
    {
        auto mapped = tdc::io::MMapReadOnlyFile(filename);
        const uint64_t* data = (const uint64_t*)mapped.data();
        
        for(size_t i = 0; i < std::extent<decltype(numbers)>::value; i++) {
            ASSERT_EQ(numbers[i], data[i]);
        }
    }
    
    // remove file
    {
        std::filesystem::remove(filename);
    }
}
