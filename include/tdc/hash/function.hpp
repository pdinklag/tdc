#pragma once

#include <cstddef>
#include <utility>

#include <tdc/util/concepts.hpp>

namespace tdc {
namespace hash {
    /// \brief A simple hash function where the hash value equals the key.
    class Identity {
    public:
        /// \brief Default constructor.
        inline Identity() {
        }
    
        /// \brief Computes the hash value for a key.
        /// \tparam key_t the key type
        /// \param key the key to hash
        template<std::integral key_t>
        inline size_t operator()(const key_t& key) const {
            return (size_t)key;
        }
    };
    
    /// \brief Modulo hashing.
    class Modulo {
    private:
        size_t m_operand;
    
    public:
        /// \brief Initializes modulo hashing.
        /// \param operand the operand
        inline Modulo(const uint64_t operand) : m_operand(operand) {
        }
        
        inline Modulo(const Modulo& other) = default;
        inline Modulo(Modulo&& other) = default;
        inline Modulo& operator=(const Modulo& other) = default;
        inline Modulo& operator=(Modulo&& other) = default;
        
        /// \brief Computes the hash value for a key.
        /// \tparam key_t the key type
        /// \param key the key to hash
        template<Arithmetic key_t>
        requires std::integral<decltype(std::declval<key_t>() % std::declval<uint64_t>())>
        inline uint64_t operator()(const key_t& key) const {
            return (uint64_t)(key % (key_t)m_operand);
        }
    };
    
    /// \brief Multiplicative hashing.
    class Multiplicative {
    public:
        /// \brief The default prime used for multiplication.
        static constexpr size_t KNUTH_PRIME = 2654435761ULL;
        
    private:
        uint64_t m_operand;

    public:
        /// \brief Default constructor.
        inline Multiplicative() : m_operand(KNUTH_PRIME) {
        }
        
        /// \brief Initializes multiplicative hashing.
        /// \param operand the operand
        inline Multiplicative(const uint64_t operand) : m_operand(operand) {
        }
        
        inline Multiplicative(const Multiplicative& other) = default;
        inline Multiplicative(Multiplicative&& other) = default;
        inline Multiplicative& operator=(const Multiplicative& other) = default;
        inline Multiplicative& operator=(Multiplicative&& other) = default;
        
        /// \brief Computes the hash value for a key.
        /// \tparam key_t the key type
        /// \param key the key to hash
        template<Arithmetic key_t>
        requires std::integral<decltype(std::declval<key_t>() * std::declval<uint64_t>())>
        inline uint64_t operator()(const key_t& key) const {
            return (uint64_t)(key * (key_t)m_operand);
        }
    };
}} // namespace tdc::hash
