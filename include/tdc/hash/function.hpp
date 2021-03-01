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
    
    /// \brief Multiplicative hashing with a prime.
    class Multiplicative {
    public:
        /// \brief The default prime used for multiplication.
        static constexpr size_t KNUTH_PRIME = 2654435761ULL;
        
    private:
        size_t m_prime;

    public:
        /// \brief Default constructor.
        inline Multiplicative() : m_prime(KNUTH_PRIME) {
        }
        
        /// \brief Initializes multiplicative hashing with a prime.
        /// \param prime the prime number. Note that no check is made to verify the given number is really a prime.
        inline Multiplicative(const size_t& prime) : m_prime(prime) {
        }
        
        inline Multiplicative(const Multiplicative& other) = default;
        inline Multiplicative(Multiplicative&& other) = default;
        inline Multiplicative& operator=(const Multiplicative& other) = default;
        inline Multiplicative& operator=(Multiplicative&& other) = default;
        
        /// \brief Computes the hash value for a key.
        /// \tparam key_t the key type
        /// \param key the key to hash
        template<Arithmetic key_t>
        requires std::integral<decltype(std::declval<key_t>() * std::declval<size_t>())>
        inline size_t operator()(const key_t& key) const {
            return (size_t)(key * (key_t)m_prime);
        }
    };
}
}
