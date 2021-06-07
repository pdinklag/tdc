#include <iostream>
#include <sstream>
#include <type_traits>

#include <tdc/framework/crossproduct.hpp>
#include <tdc/framework/registry.hpp>

#include <tdc/util/type_list.hpp>

namespace tdc {

namespace code {

    class Code : public framework::Algorithm {
    };

    template<typename T>
    concept Code = std::is_base_of<Code, T>::value;

    class Binary : public Code {
    public: virtual std::string id() const override { return "Binary"; }
    };

    class EliasDelta : public Code {
    public: virtual std::string id() const override { return "EliasDelta"; }
    };

    class Huffman : public Code {
    public: virtual std::string id() const override { return "Huffman"; }
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
    public: virtual std::string id() const override { return "Binary"; }
    };

    class BinaryTrieMTF : public LZ78Trie {
    public: virtual std::string id() const override { return "BinaryMTF"; }
    };

    template<code::Code RefCode, code::Code CharCode>
    class RefCharTuples : public LZ78Code {
    private:
        RefCode ref_code_;
        CharCode char_code_;
        
    public:
        virtual std::string id() const override {
            std::ostringstream s;
            s << "RefCharTuples<" << ref_code_.id() << ", " << char_code_.id() << ">";
            return s.str();
        }
    };

    template<code::Code RefCode, code::Code CharCode>
    class RefCharArrays : public LZ78Code {
    private:
        RefCode ref_code_;
        CharCode char_code_;
        
    public:
        virtual std::string id() const override {
            std::ostringstream s;
            s << "RefCharArrays<" << ref_code_.id() << ", " << char_code_.id() << ">";
            return s.str();
        }
    };
} // namespace lz78

template<LZ78Code Code, LZ78Trie Trie>
class LZ78 : public framework::Algorithm {
private:
    Code code_;
    Trie trie_;

public:
    virtual std::string id() const override {
        std::ostringstream s;
        s << "LZ78<" << code_.id() << ", " << trie_.id() << ">";
        return s.str();
    }
};

} // namespace tdc

using namespace tdc;

int main(int argc, char** argv) {
    using UniversalCodes = tl::list<code::Binary, code::EliasDelta>;
    using OfflineCodes = tl::list<code::Huffman>;
    using AllCodes = tl::concat<UniversalCodes, OfflineCodes>;
    
    using LZ78Codes = tl::concat<
        framework::crossproduct<lz78::RefCharTuples, UniversalCodes, AllCodes>,
        framework::crossproduct<lz78::RefCharArrays, UniversalCodes, AllCodes>
    >;
    using LZ78Tries = tl::list<lz78::BinaryTrie, lz78::BinaryTrieMTF>;
    
    using Types = framework::crossproduct<LZ78, LZ78Codes, LZ78Tries>;

    framework::Registry r;
    r.register_algorithms(Types());

    /*



    framework::Registry r;
    r.register_algorithms(framework::crossproduct<LZ78, LZ78Codes, LZ78Tries>());
    */
}
