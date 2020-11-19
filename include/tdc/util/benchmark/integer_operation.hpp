#pragma once

#include <iostream>
#include <vector>
#include <utility>

namespace tdc {
namespace benchmark {

/// \brief The type for operation counts.
using opcode_t = char;

/// \brief An operation code for \em insert operations.
constexpr opcode_t OPCODE_INSERT = 'I';

/// \brief An operation code for \em delete operations.
constexpr opcode_t OPCODE_DELETE = 'D';

/// \brief An operation code for \em query operations.
constexpr opcode_t OPCODE_QUERY = 'Q';

template<typename key_t>
class IntegerOperationBatch {
private:
    using batchsize_t = uint32_t;

    opcode_t           m_opcode;
    std::vector<key_t> m_keys;

public:
    static IntegerOperationBatch read(std::istream& in) {
        opcode_t opcode;
        batchsize_t num_keys;
        
        in >> opcode;
        in.read((char*)&num_keys, sizeof(num_keys));
        
        IntegerOperationBatch batch(opcode, num_keys);
        batch.m_keys.resize(num_keys);
        in.read((char*)batch.m_keys.data(), num_keys * sizeof(key_t));
    }

    IntegerOperationBatch(const opcode_t opcode, const size_t capacity) : m_opcode(opcode) {
        m_keys.reserve(capacity);
    }

    opcode_t opcode() const {
        return m_opcode;
    }
    
    const std::vector<key_t>& keys() const {
        return m_keys;
    }
    
    size_t size() const {
        return m_keys.size();
    }
    
    void add_key(key_t&& key) {
        m_keys.emplace_back(std::move(key));
    }
    
    void write(std::ostream& out) const {
        out << m_opcode;
        
        const batchsize_t num_keys = m_keys.size();
        out.write((const char*)&num_keys, sizeof(num_keys));
        out.write((const char*)m_keys.data(), num_keys * sizeof(key_t));
    }
};

}} // namespace tdc::benchmark
