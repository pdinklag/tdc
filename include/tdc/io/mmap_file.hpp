#pragma once

#include <cstddef>
#include <string>

namespace tdc {
namespace io {

/// \brief A memory mapped read-only file.
class MMapReadOnlyFile {
private:
    int m_fd;
    void* m_data;
    size_t m_size;

public:
    /// \brief Maps a read-only file to memory.
    MMapReadOnlyFile(const std::string& filename);
    
    /// \brief Unmaps and closes the file.
    ~MMapReadOnlyFile();

    /// \brief Returns the pointer to the mapped file contents.
    inline const void* data() const { return m_data; }
    
    /// \brief Gets the size of the file.
    inline size_t size() const { return m_size; }
};

}} // namespace tdc::io
