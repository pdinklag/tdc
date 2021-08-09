#pragma once

#include <iostream>

namespace tdc {
namespace io {

template<typename item_t>
class BufferedWriter {
private:
    std::ostream* m_stream;
    item_t* m_buffer;
    size_t m_bufsize;
    
    size_t m_cursor;

public:
    BufferedWriter(std::ostream& stream, const size_t bufsize) : m_cursor(0), m_stream(&stream), m_bufsize(bufsize) {
        m_buffer = new item_t[bufsize];
    }
    
    ~BufferedWriter() {
        flush();
        delete[] m_buffer;
    }

    void write(item_t x) {
        if(m_cursor >= m_bufsize) {
            flush();
        }
        m_buffer[m_cursor++] = x;
    }

    void flush() {
        m_stream->write((char*)m_buffer, m_cursor * sizeof(item_t));
        m_cursor = 0;
    }
};

}} // namespace tdc::io
