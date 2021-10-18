#include <iostream>
#include <sstream>
#include <type_traits>

#include <tdc/framework/executable.hpp>
#include <tdc/framework/application.hpp>

#include <tdc/util/type_list.hpp>

namespace tdc {

namespace code {

    class Code : public framework::Algorithm {
    };

    template<typename T>
    concept Code = std::is_base_of<Code, T>::value;

    class Binary : public Code {
    public:
        static inline framework::AlgorithmInfo info() {
            return framework::AlgorithmInfo("Binary");
        }
    };

    class EliasDelta : public Code {
    public:
        static inline framework::AlgorithmInfo info() {
            return framework::AlgorithmInfo("EliasDelta");
        }
    };

    class Huffman : public Code {
    public:
        static inline framework::AlgorithmInfo info() {
            return framework::AlgorithmInfo("Huffman");
        }
    };

} // namespace code

class LZ78Trie : public framework::Algorithm {
};

template<typename T>
concept LZ78Trie = std::is_base_of<LZ78Trie, T>::value;

class LZ78Code : public framework::Algorithm {
};

template<typename T>
concept LZ78Code = std::is_base_of<LZ78Code, T>::value;

namespace lz78 {
    class BinaryTrie : public LZ78Trie {
    public:
        static inline framework::AlgorithmInfo info() {
            return framework::AlgorithmInfo("BinaryTrie");
        }
    };

    class BinaryTrieMTF : public LZ78Trie {
    public:
        static inline framework::AlgorithmInfo info() {
            return framework::AlgorithmInfo("BinaryTrieMTF");
        }
    };

    template<code::Code RefCode, code::Code CharCode>
    class RefCharTuples : public LZ78Code {
    private:
        RefCode ref_code_;
        CharCode char_code_;

    public:
        static inline framework::AlgorithmInfo info() {
            framework::AlgorithmInfo info("RefCharTuples");
            info.add_sub_algorithm<RefCode>("ref");
            info.add_sub_algorithm<CharCode>("char");
            return info;
        }
    };

    template<code::Code RefCode, code::Code CharCode>
    class RefCharArrays : public LZ78Code {
    private:
        RefCode ref_code_;
        CharCode char_code_;
        
    public:
        static inline framework::AlgorithmInfo info() {
            framework::AlgorithmInfo info("RefCharArrays");
            info.add_sub_algorithm<RefCode>("ref");
            info.add_sub_algorithm<CharCode>("char");
            return info;
        }
    };
} // namespace lz78

template<LZ78Code Code, LZ78Trie Trie>
class LZ78 : public framework::Executable {
private:
    Code code_;
    Trie trie_;

public:
    static inline framework::AlgorithmInfo info() {
        framework::AlgorithmInfo info("LZ78");
        info.add_sub_algorithm<Code>("code");
        info.add_sub_algorithm<Trie>("trie");
        return info;
    }

    virtual int execute(int& in, int& out) override {
        std::cout << "Hello, world!" << std::endl;
        return 0;
    }
};

} // namespace tdc

using namespace tdc;

int main(int argc, char** argv) {
    using UniversalCodes = tl::list<code::Binary, code::EliasDelta>;
    using OfflineCodes = tl::list<code::Huffman>;
    using AllCodes = tl::concat<UniversalCodes, OfflineCodes>;
    
    using LZ78Codes = tl::concat<
        tl::template_instances<lz78::RefCharTuples, UniversalCodes, AllCodes>,
        tl::template_instances<lz78::RefCharArrays, UniversalCodes, AllCodes>
    >;
    using LZ78Tries = tl::list<lz78::BinaryTrie, lz78::BinaryTrieMTF>;
    
    using Types = tl::template_instances<LZ78, LZ78Codes, LZ78Tries>;

   framework::Application app;
   app.default_executable<LZ78<lz78::RefCharArrays<code::Binary, code::Huffman>, lz78::BinaryTrieMTF>>();
   app.register_executables(Types());
   return app.run(argc, argv);
}
