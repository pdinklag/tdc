#pragma once

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
        template<typename key_t>
        inline size_t operator()(const key_t& key) {
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
        
        /// \brief Computes the hash value for a key.
        /// \tparam key_t the key type
        /// \param key the key to hash
        template<typename key_t>
        inline size_t operator()(const key_t& key) {
            return (size_t)(key * m_prime);
        }
    };
}
}
