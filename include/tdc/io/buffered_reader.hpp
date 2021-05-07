#pragma once

#include <iostream>

namespace tdc {
namespace io {

template<typename item_t>
class BufferedReader {
private:
    std::istream* m_stream;
    item_t* m_buffer;
    size_t m_bufsize_bytes;
    
    size_t m_count;
    size_t m_cursor;

    bool underflow() {
        // underflow
        m_stream->read((char*)m_buffer, m_bufsize_bytes);
        m_count = m_stream->gcount() / sizeof(item_t);
        m_cursor = 0;
        return m_count > 0;
    }

public:
    BufferedReader(std::istream& stream, const size_t bufsize) : m_stream(&stream), m_bufsize_bytes(bufsize * sizeof(item_t)) {
        m_buffer = new item_t[bufsize];
        underflow();
    }
    
    ~BufferedReader() {
        delete[] m_buffer;
    }
    
    operator bool() const {
        return m_cursor < m_count || *m_stream;
    }
    
    item_t read() {
        if(m_cursor >= m_count) {
            if(!underflow()) return 0;
        }
        return m_buffer[m_cursor++];
    }
};

}} // namespace tdc::io
