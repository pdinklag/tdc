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

template<size_t B, std::unsigned_integral qgram_t = uint64_t, bool m_binary_search = false>
class BTreeProcessor {
private:
    template<std::unsigned_integral char_t>
    static size_t compute_lcp(qgram_t a, qgram_t b) {
        return intrisics::lzcnt0(a ^ b) / std::numeric_limits<char_t>::digits;
    }

    struct BTreeEntry {
        qgram_t qgram;
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
    } __attribute__((__packed__));
    
    pred::dynamic::BTree<BTreeEntry, B+1, pred::dynamic::SortedArrayNode<BTreeEntry, B, m_binary_search>> m_btree;

public:
    template<typename lzqgram_t>
    requires (lzqgram_t::qgram_endian == std::endian::little)
    void process(lzqgram_t& c, std::ostream& out) {
        const auto qgram = c.qgram();
        // std::cout << "qgram = 0x" << std::hex << qgram << std::endl;
        
        size_t lcp = 0;
        size_t src = 0;
        
        // find predecessor
        auto pred = m_btree.predecessor(BTreeEntry(qgram));
        // if(pred.exists) std::cout << "\tfound predecessor 0x" << std::hex << pred.key.qgram << std::endl;
        
        bool exact_match = false;
        if(pred.exists && pred.key.qgram == qgram) {
            // std::cout << "\texact match!" << std::endl;
            lcp = lzqgram_t::q;
            src = pred.key.last_seen_at;
            exact_match = true;
        } else {
            // find successor
            // TODO: this could be much faster using an iterator...
            auto succ = m_btree.successor(BTreeEntry(qgram));
            // if(succ.exists) std::cout << "\tfound successor 0x" << std::hex << pred.key.qgram << std::endl;
            
            if(pred.exists && succ.exists) {
                // both predecessor and successor exist, take the one with longer LCP
                const size_t lcp_pred = compute_lcp<typename lzqgram_t::char_t>(qgram, pred.key.qgram);
                const size_t lcp_succ = compute_lcp<typename lzqgram_t::char_t>(qgram, succ.key.qgram);
                if(lcp_pred > lcp_succ) {
                    lcp = lcp_pred;
                    src = pred.key.last_seen_at;
                } else {
                    lcp = lcp_succ;
                    src = succ.key.last_seen_at;
                }
            } else if(pred.exists) {
                lcp = compute_lcp<typename lzqgram_t::char_t>(qgram, pred.key.qgram);
                src = pred.key.last_seen_at;
            } else if(succ.exists) {
                lcp = compute_lcp<typename lzqgram_t::char_t>(qgram, succ.key.qgram);
                src = succ.key.last_seen_at;
            } else {
                // neither predecessor nor successor have been found
            }
        }
        
        c.output_ref(out, src, lcp);

        // enter
        if(!exact_match) m_btree.insert(BTreeEntry(qgram, c.pos()));
    }
};

}}}} // namespace tdc::comp::lz77::qgram
