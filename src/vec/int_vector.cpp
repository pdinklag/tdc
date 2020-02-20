#include <tdc/vec/int_vector.hpp>

using namespace tdc::vec;

std::unique_ptr<uint64_t[]> IntVector::allocate(const size_t num, const size_t w) {
    const size_t num64 = math::idiv_ceil(num * w, 64ULL);
    uint64_t* p = new uint64_t[num64];
    memset(p, 0, num64 * sizeof(uint64_t));
    return std::unique_ptr<uint64_t[]>(p);
}

uint64_t IntVector::get(const size_t i) const {
    const size_t j = i * m_width;
    const size_t a = j >> 6ULL;                    // left border
    const size_t b = (j + m_width - 1ULL) >> 6ULL; // right border

    // da is the distance of a's relevant bits from the left border
    const size_t da = j & 63ULL;

    // wa is the number of a's relevant bits
    const size_t wa = 64ULL - da;

    // get the wa highest bits from a
    const uint64_t a_hi = m_data[a] >> da;

    // get b (its high bits will be masked away below)
    // NOTE: we could save this step if we knew a == b,
    //       but the branch caused by checking that is too expensive
    const uint64_t b_lo = m_data[b];

    // combine
    return ((b_lo << wa) | a_hi) & m_mask;
}

void IntVector::set(const size_t i, const uint64_t v_) {
    const uint64_t v = v_ & m_mask; // make sure it fits...
    
    const size_t j = i * m_width;
    const size_t a = j >> 6ULL;       // left border
    const size_t b = (j + m_width - 1ULL) >> 6ULL; // right border
    if(a < b) {
        // the bits are the suffix of m_data[a] and prefix of m_data[b]
        const size_t da = j & 63ULL;
        const size_t wa = 64ULL - da;
        const size_t wb = m_width - wa;
        const size_t db = 64ULL - wb;

        // combine the da lowest bits from a and the wa lowest bits of v
        const uint64_t a_lo = m_data[a] & bit_mask(da);
        const uint64_t v_lo = v & bit_mask(wa);
        m_data[a] = (v_lo << da) | a_lo;

        // combine the db highest bits of b and the wb highest bits of v
        const uint64_t b_hi = m_data[b] >> wb;
        const uint64_t v_hi = v >> wa;
        m_data[b] = (b_hi << wb) | v_hi;
    } else {
        const size_t dl = j & 63ULL;
        const size_t dvl = dl + m_width;
        
        const uint64_t xa = m_data[a];
        const uint64_t lo = xa & bit_mask(dl);
        const uint64_t hi = xa >> dvl;
        m_data[a] = lo | (v << dl) | (hi << dvl);
    }
}

void IntVector::resize(const size_t size, const size_t width) {
    IntVector new_iv(size, width);
    for(size_t i = 0; i < size; i++) {
        new_iv.set(i, get(i));
    }
    
    m_size = new_iv.m_size;
    m_width = new_iv.m_width;
    m_mask = new_iv.m_mask;
    m_data = std::move(new_iv.m_data);
}

