#include <tdc/uint/uint256.hpp>
#include <tdc/math/ilog2.hpp>

namespace tdc {

constexpr uint64_t lo64(const uint128_t& x) { return x; }
constexpr uint64_t hi64(const uint128_t& x) { return (x >> 64); }

uint256_t uint256_t::operator*(const uint256_t& other) const {
    if(*this == 0 || other == 0) {
        return uint256_t(0);
    } else if(other == 1) {
        return *this;
    } else if(*this == 1) {
        return other;
    }
    
    // borrowed from Github: calccrypto/uint256_t (MIT license)
    
    // split values into 4 64-bit parts
    uint128_t top[4] = { hi64(m_hi), lo64(m_hi), hi64(m_lo), lo64(m_lo)};
    uint128_t bottom[4] = { hi64(other.m_hi), lo64(other.m_hi), hi64(other.m_lo), lo64(other.m_lo)};
    uint128_t products[4][4];

    // multiply each component of the values
    for(int y = 3; y >= 0; y--){
        for(int x = 3; x >= 0; x--){
            products[3 - y][x] = top[x] * bottom[y];
        }
    }

    // first row
    uint128_t fourth64 = uint128_t(lo64(products[0][3]));
    uint128_t third64  = uint128_t(lo64(products[0][2])) + uint128_t(hi64(products[0][3]));
    uint128_t second64 = uint128_t(lo64(products[0][1])) + uint128_t(hi64(products[0][2]));
    uint128_t first64  = uint128_t(lo64(products[0][0])) + uint128_t(hi64(products[0][1]));

    // second row
    third64  += uint128_t(lo64(products[1][3]));
    second64 += uint128_t(lo64(products[1][2])) + uint128_t(hi64(products[1][3]));
    first64  += uint128_t(lo64(products[1][1])) + uint128_t(hi64(products[1][2]));

    // third row
    second64 += uint128_t(lo64(products[2][3]));
    first64  += uint128_t(lo64(products[2][2])) + uint128_t(hi64(products[2][3]));

    // fourth row
    first64  += uint128_t(lo64(products[3][3]));

    // combines the values, taking care of carry over
    return uint256_t(first64 << 64, 0) +
           uint256_t(hi64(third64), third64 << 64) +
           uint256_t(second64, 0) +
           uint256_t(fourth64);
}

std::pair<uint256_t, uint256_t> uint256_t::divmod(const uint256_t& a, const uint256_t& b) {
    // borrowed from Github: calccrypto/uint256_t (MIT license)
    if (b == 0){
        throw std::domain_error("Error: division or modulus by 0");
    } else if (b == 1) {
        return std::pair<uint256_t, uint256_t>(a, 0);
    } else if (a == b){
        return std::pair<uint256_t, uint256_t>(1, 0);
    } else if (a == 0 || a < b){
        return std::pair<uint256_t, uint256_t>(0, a);
    }

    std::pair<uint256_t, uint256_t> qr(0, a);
    
    const size_t bits_a = math::ilog2_ceil(a);
    const size_t bits_b = math::ilog2_ceil(b);
    
    uint256_t copyd = b << (bits_a - bits_b);
    uint256_t adder = uint256_t(1) << (bits_a - bits_b);
    
    if (copyd > qr.second){
        copyd >>= 1;
        adder >>= 1;
    }
    
    while (qr.second >= b){
        if (qr.second >= copyd) {
            qr.second -= copyd;
            qr.first |= adder;
        }
        copyd >>= 1;
        adder >>= 1;
    }
    return qr;
}

} // namespace tdc
