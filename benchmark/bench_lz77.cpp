#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include <tdc/comp/lz77/factor_buffer.hpp>
#include <tdc/comp/lz77/factor_file_output.hpp>
#include <tdc/comp/lz77/factor_multi_output.hpp>
#include <tdc/comp/lz77/factor_readable_output.hpp>
#include <tdc/comp/lz77/factor_stats_output.hpp>

#include <tdc/comp/lz77/noop.hpp>
#include <tdc/comp/lz77/lz77_sa.hpp>
#include <tdc/comp/lz77/lz77_sw.hpp>
#include <tdc/comp/lz77/lzfp.hpp>
#include <tdc/comp/lz77/lzsketch.hpp>

#include <tdc/uint/uint128.hpp>
#include <tdc/uint/uint256.hpp>
#include <tdc/io/buffered_reader.hpp>
#include <tdc/io/null_ostream.hpp>
#include <tdc/stat/phase.hpp>
#include <tdc/util/literals.hpp>

#include <tlx/cmdline_parser.hpp>

using namespace tdc::comp::lz77;

using char_t = unsigned char;

struct {
    std::string filename;
    size_t q = 0;
    size_t window = 1_Ki;
    size_t tau_min = 5;
    size_t tau_max = 5;
    size_t filter_size = 1_Ki;
    size_t cm_width = 10;
    size_t cm_height = 2;
    
    bool merge = false;
    std::vector<std::string> merge_files;
    
    std::string roundtrip;
    std::string ds;
    
    bool do_bench(const std::string& group) {
        return ds.length() == 0 || ds == group;
    }
} options;

template<typename ctor_t>
void bench(const std::string& group, std::string&& name, ctor_t ctor, bool can_merge = true) {
    if(!options.do_bench(group)) return;

    const size_t file_size = std::filesystem::file_size(options.filename);
    std::ifstream input(options.filename);
    tdc::io::NullOStream devnull;
    
    {
        tdc::stat::Phase phase("compress");

        FactorStatsOutput factors;

        {
            auto c = ctor();
            if(options.roundtrip.length() > 0) {
                FactorBuffer buf;
                {
                    std::ofstream fout(options.roundtrip);
                    FactorReadableOutput file_output(fout);
                    FactorMultiOutput multi(factors, buf, file_output);
                    c.compress(input, multi);
                }

                // decode
                {
                    std::ofstream fdec(options.roundtrip + ".dec");
                    fdec << buf.decode();
                }
            } else if(options.merge && can_merge) {
                std::ofstream fout(name);
                tdc::io::BufferedWriter<Factor> factors_writer(fout, 10_Ki);
                FactorFileOutput file_output(factors_writer);
                FactorMultiOutput multi(factors, file_output);
                c.compress(input, multi);
            } else {
                c.compress(input, factors);
            }

            auto guard = phase.suppress();
            c.log_stats(phase);
        }

        auto guard = phase.suppress();
        const auto& stats = factors.stats();

        phase.log("input_size", stats.input_size);
        phase.log("num_literals", stats.num_literals);
        phase.log("num_refs", stats.num_refs);
        phase.log("num_factors", factors.size());
        phase.log("min_ref_len", stats.min_ref_len);
        phase.log("max_ref_len", stats.max_ref_len);
        phase.log("avg_ref_len", std::lround((double)stats.total_ref_len / (double)stats.num_refs));
        phase.log("min_ref_dist", stats.min_ref_dist);
        phase.log("max_ref_dist", stats.max_ref_dist);
        phase.log("avg_ref_dist", std::lround((double)stats.total_ref_dist / (double)stats.num_refs));
        std::cout << "RESULT algo=" << name << " group=" << group << " input=" << options.filename << " " << phase.to_keyval() << std::endl;

        assert(stats.input_size == file_size);

        if(options.merge && can_merge) {
            options.merge_files.push_back(name);
        }
    }
}

FactorBuffer read_factors(const std::string& file) {
    std::ifstream in(file);
    tdc::io::BufferedReader<Factor> r(in, 1_Mi);

    FactorBuffer buf;
    while(r) {        
        buf.emplace_back(r.read());
    }
    return buf;
}

void print_merge_result(const FactorStatsOutput& factors) {
    const auto& stats = factors.stats();
    std::cout << " input_size=" << stats.input_size;
    std::cout << " num_literals=" << stats.num_literals;
    std::cout << " num_refs=" << stats.num_refs;
    std::cout << " num_factors=" << factors.size();
    std::cout << " min_ref_len=" << stats.min_ref_len;
    std::cout << " max_ref_len=" << stats.max_ref_len;
    std::cout << " avg_ref_len=" << std::lround((double)stats.total_ref_len / (double)stats.num_refs);
    std::cout << " min_ref_dist=" << stats.min_ref_dist;
    std::cout << " max_ref_dist=" << stats.max_ref_dist;
    std::cout << " avg_ref_dist=" << std::lround((double)stats.total_ref_dist / (double)stats.num_refs);
}   

void merge(const std::string& file1, const std::string& file2) {
    if(std::filesystem::is_regular_file(file1) && std::filesystem::is_regular_file(file2)) {
        FactorStatsOutput factors;
        {
            auto f1 = read_factors(file1);
            auto f2 = read_factors(file2);
            FactorBuffer::merge(f1, f2, factors);
        }
        
        std::cout << "RESULT algo=Merge(" << file1 << "," << file2 << ") input=" << options.filename;
        print_merge_result(factors);
        std::cout << std::endl;
    }
}

void merge(const std::string& file1, const std::string& file2, const std::string& file3) {
    if(std::filesystem::is_regular_file(file1) && std::filesystem::is_regular_file(file2) && std::filesystem::is_regular_file(file3)) {
        FactorStatsOutput factors;
        {
            FactorBuffer temp;
            {
                auto f2 = read_factors(file2);
                auto f3 = read_factors(file3);
                FactorBuffer::merge(f2, f3, temp);
            }

            auto f1 = read_factors(file1);
            FactorBuffer::merge(f1, temp, factors);
        }
        
        std::cout << "RESULT algo=Merge(" << file1 << "," << file2 << "," << file3 << ") input=" << options.filename;
        print_merge_result(factors);
        std::cout << std::endl;
    }
}

int main(int argc, char** argv) {
    tlx::CmdlineParser cp;
    cp.add_param_string("file", options.filename, "The input file.");
    cp.add_string('a', "group", options.ds, "The algorithm group to benchmark.");
    cp.add_size_t('q', "min-qgram", options.q, "The minimum q-gram length.");
    cp.add_bytes('w', "window", options.window, "The window length for sliding window algorithms (default: 1024).");
    cp.add_bytes("tau-min", options.tau_min, "The minimum window length exponent for fingerprinting algorithms (default: 5, pertaining to 32).");
    cp.add_bytes("tau-max", options.tau_max, "The maximum window length exponent for fingerprinting algorithms (default: 5, pertaining to 32).");
    cp.add_bytes("filter", options.filter_size, "The size of the sketch filter if used (default: 1024).");
    cp.add_bytes("cm-width", options.cm_width, "The width exponent of the count-min sketch if used (default: 10).");
    cp.add_bytes("cm-height", options.cm_height, "The height of the count-min sketch if used (default: 4).");
    cp.add_flag("merge", options.merge, "Simulates merging of factorizations.");
    cp.add_string("roundtrip", options.roundtrip, "Outputs the factorization to the specified file and decodes it afterwards.");
    
    if(!cp.process(argc, argv)) {
        return -1;
    }

    if(options.merge && options.roundtrip.length() > 0) {
        std::cerr << "Roundtrip not supported in combination with merge" << std::endl;
        return -1;
    }

    options.tau_max = std::max(options.tau_min, options.tau_max);

    //~ bench("base", "Noop", [](){ return Noop(); });
    bench("base", "SA", [](){ return LZ77SA(); }, false);
    bench("sliding", "Sliding", [](){ return LZ77SlidingWindow<false>(options.window); });
    bench("fp", "FP", [](){ return LZFingerprinting(options.tau_min, options.tau_max); });

    if(options.q == 0 || options.q == 8) {
        bench("sketch", "Sketch(q=8)", [](){ return LZSketch<uint64_t>(options.filter_size, 1ULL << options.cm_width, options.cm_height); });
    }
    
    if(options.q == 0 || options.q == 16) {
        bench("sketch", "Sketch(q=16)", [](){ return LZSketch<uint128_t>(options.filter_size, 1ULL << options.cm_width, options.cm_height); });
    }

    if(options.q == 0 || options.q == 32) {
        bench("sketch", "Sketch(q=32)", [](){ return LZSketch<uint256_t>(options.filter_size, 1ULL << options.cm_width, options.cm_height); });
    }

    if(options.merge) {
        merge("Sliding", "FP");
        merge("Sliding", "Sketch(q=8)");
        merge("Sliding", "Sketch(q=16)");
        merge("Sliding", "Sketch(q=32)");
        merge("FP", "Sketch(q=8)");
        merge("FP", "Sketch(q=16)");
        merge("FP", "Sketch(q=32)");
        merge("Sliding", "FP", "Sketch(q=8)");
        merge("Sliding", "FP", "Sketch(q=16)");
        merge("Sliding", "FP", "Sketch(q=32)");

        for(const auto& file : options.merge_files) {
            std::filesystem::remove(file);
        }
    }
}
