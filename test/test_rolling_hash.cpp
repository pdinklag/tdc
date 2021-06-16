#include <iostream>

#include <tdc/hash/rolling.hpp>
#include <tdc/test/assert.hpp>

using RollingHash = tdc::hash::RollingKarpRabinFingerprint<unsigned char>;

void test(const size_t w) {
    assert(w % 2 == 0); // must be even for this test
    std::cout << "test w=" << w << std::endl;
    
    // create RH for w symbols
    RollingHash h(w);

    // insert w symbols
    for(size_t i = 0; i < w; i++) {
        h.advance('a' + i, 0);
    }
    
    // save fingerprint
    const auto first = h.fingerprint();
    std::cout << "\tfirst=" << first << std::endl;

    // insert w/2 first symbols again
    for(size_t i = 0; i < w/2; i++) {
        h.advance('a' + i, 'a' + i);
    }
    ASSERT_NEQ(first, h.fingerprint());

    // insert next w/2 symbols again
    for(size_t i = 0; i < w/2; i++) {
        h.advance('a' + w/2 + i, 'a' + w/2 + i);
    }
    std::cout << "\th1=" << h.fingerprint() << std::endl;
    ASSERT_EQ(first, h.fingerprint());

    // insert symbols into a new instance
    RollingHash h2(w);
    for(size_t i = 0; i < w; i++) {
        h2.advance('a' + i, 0);
    }
    std::cout << "\th2=" << h2.fingerprint() << std::endl;
    ASSERT_EQ(h.fingerprint(), h2.fingerprint());
}

int main(int argc, char** argv) {
    test(8);
    test(16);
    test(32);
    test(64);
    test(128);
}
