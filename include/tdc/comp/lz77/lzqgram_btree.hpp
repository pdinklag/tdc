#pragma once

#include <bit>
#include <cstdint>
#include <concepts>
#include <iostream>
#include <limits>

#include <tdc/intrisics/lzcnt.hpp>
#include <tdc/pred/dynamic/btree.hpp>
#include <tdc/pred/dynamic/btree/sorted_array_node.hpp>
#include <tdc/util/index.hpp>

#include "stats.hpp"

namespace tdc {
namespace comp {
namespace lz77 {
namespace qgram {

template<size_t B, std::unsigned_integral qgram_t = uint64_t>
class BTreeProcessor {
private:
    struct BTreeEntry {
        qgram_t qgram;
        char term = 0; // TODO DEBUG
        index_t last_seen_at;
        
        BTreeEntry() : qgram(0), last_seen_at(0) {
        }
        
        BTreeEntry(qgram_t _qgram) : qgram(_qgram), last_seen_at(0) {
        }
        
        BTreeEntry(qgram_t _qgram, index_t _last_seen_at) : qgram(_qgram), last_seen_at(_last_seen_at) {
        }
        
        auto operator<=>(const BTreeEntry& e) const { return qgram <=> e.qgram; }
        bool operator==(const BTreeEntry& e) const { return qgram == e.qgram; }
        bool operator!=(const BTreeEntry& e) const { return qgram != e.qgram; }
    };
    
    pred::dynamic::BTree<BTreeEntry, B+1, pred::dynamic::SortedArrayNode<BTreeEntry, B, false>> m_btree;

public:
    template<typename lzqgram_t>
    requires (lzqgram_t::qgram_endian == std::endian::little)
    void process(lzqgram_t& c, std::ostream& out) {
        const auto qgram = c.qgram();
        std::cout << "qgram = 0x" << std::hex << qgram << std::endl;
        
        // find predecessor
        // TODO: also find successor!
        
        auto r = m_btree.predecessor(BTreeEntry(qgram));
        if(r.exists) {
            std::cout << "\tfound predecessor 0x" << std::hex << r.key.qgram << std::endl;
            
            const size_t lcp = intrisics::lzcnt0(qgram ^ r.key.qgram) / lzqgram_t::char_bits;
            std::cout << "\tlcp = " << std::dec << lcp << std::endl;
            
            c.output_ref(out, r.key.last_seen_at, lcp);
            
            if(r.key.qgram == qgram) {
                // remove existing entry
                m_btree.remove(BTreeEntry(qgram));
                std::cout << "\texact match!" << std::endl;
            }
        } else {
            std::cout << "\tno predecessor found" << std::endl;
        }
        
        // enter
        std::cout << "\tenter 0x" << std::hex << qgram << std::endl;
        m_btree.insert(BTreeEntry(qgram, c.pos()));
    }
};

}}}} // namespace tdc::comp::lz77::qgram
