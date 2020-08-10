#include <tdc/io/mmap_file.hpp>

#include <cstdlib>

#include <fcntl.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/mman.h>

using namespace tdc::io;

MMapReadOnlyFile::MMapReadOnlyFile(const std::string& filename) : m_data(nullptr), m_size(0) {
    m_fd = open(filename.c_str(), O_RDONLY);
    if(m_fd >= 0) {
        struct stat st;
        if(fstat(m_fd, &st) >= 0) {
            void* data = mmap(nullptr, st.st_size, PROT_READ, MAP_PRIVATE, m_fd, 0U);
            if(data != MAP_FAILED) {
                m_data = data;
                m_size = st.st_size;
            }
        }
    }
}

MMapReadOnlyFile::~MMapReadOnlyFile() {
    if(m_data) {
        munmap(m_data, m_size);
    }
    
    if(m_fd >= 0) {
        close(m_fd);
    }
}
