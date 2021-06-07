#include <iostream>
#include <sstream>
#include <type_traits>

#include <tdc/util/type_list.hpp>

namespace tdc {

namespace framework {
    class Algorithm {
    public:
        virtual std::string name() const = 0;
    };

    template<typename T>
    concept Algorithm = std::is_base_of<Algorithm, T>::value;
    
    template<Algorithm A>
    void register_algorithm() {
        A c;
        std::cout << c.name() << std::endl;
    }

    void register_algorithms(tl::list<>) {
        // done
    }

    template<Algorithm A, Algorithm... As>
    void register_algorithms(tl::list<A, As...>) {
        register_algorithm<A>();
        register_algorithms(tl::list<As...>());
    }

    template<template<typename, typename> typename A, typename Tl1, typename Tl2>
    struct _crossproduct2;

    template<template<typename, typename> typename A, typename Tl1, typename Tl2>
    using crossproduct2 = _crossproduct2<A, Tl1, Tl2>::type_list;

    template<template<typename, typename> typename A>
    struct _crossproduct2<A, tl::empty, tl::empty> {
        using type_list = tl::empty;
    };

    template<template<typename, typename> typename A, typename Tl1>
    struct _crossproduct2<A, Tl1, tl::empty> {
        using type_list = tl::empty;
    };

    template<template<typename, typename> typename A, typename Tl2>
    struct _crossproduct2<A, tl::empty, Tl2> {
        using type_list = tl::empty;
    };

    template<template<typename, typename> typename A, typename X, typename Y, typename... Ys>
    struct _crossproduct2<A, tl::list<X>, tl::list<Y, Ys...>> {
        using type_list = tl::concat<
            tl::list<A<X, Y>>,
            crossproduct2<A, tl::list<X>, tl::list<Ys...>>
        >;
    };

    template<template<typename, typename> typename A, typename X, typename... Xs, typename Y, typename... Ys>
    struct _crossproduct2<A, tl::list<X, Xs...>, tl::list<Y, Ys...>> {
        using type_list = tl::concat<
            crossproduct2<A, tl::list<X>, tl::list<Y, Ys...>>,
            crossproduct2<A, tl::list<Xs...>, tl::list<Y, Ys...>>
        >;
    };
} // namespace framework

namespace code {

    class Code : public framework::Algorithm {
    };

    template<typename T>
    concept Code = std::is_base_of<Code, T>::value;

    class Binary : public Code {
    public: virtual std::string name() const override { return "Binary"; }
    };

    class EliasDelta : public Code {
    public: virtual std::string name() const override { return "EliasDelta"; }
    };

    class Huffman : public Code {
    public: virtual std::string name() const override { return "Huffman"; }
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
    public: virtual std::string name() const override { return "Binary"; }
    };

    class BinaryTrieMTF : public LZ78Trie {
    public: virtual std::string name() const override { return "BinaryMTF"; }
    };

    template<code::Code RefCode, code::Code CharCode>
    class RefCharTuples : public LZ78Code {
    private:
        RefCode ref_code_;
        CharCode char_code_;
        
    public:
        virtual std::string name() const override {
            std::ostringstream s;
            s << "RefCharTuples<" << ref_code_.name() << ", " << char_code_.name() << ">";
            return s.str();
        }
    };

    template<code::Code RefCode, code::Code CharCode>
    class RefCharArrays : public LZ78Code {
    private:
        RefCode ref_code_;
        CharCode char_code_;
        
    public:
        virtual std::string name() const override {
            std::ostringstream s;
            s << "RefCharArrays<" << ref_code_.name() << ", " << char_code_.name() << ">";
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
    virtual std::string name() const override {
        std::ostringstream s;
        s << "LZ78<" << code_.name() << ", " << trie_.name() << ">";
        return s.str();
    }
};

} // namespace tdc

int main(int argc, char** argv) {
    using namespace tdc;

    using UniversalCodes = tl::list<code::Binary, code::EliasDelta>;
    using OfflineCodes = tl::list<code::Huffman>;
    using AllCodes = tl::concat<UniversalCodes, OfflineCodes>;

    using LZ78Codes = tl::concat<
        framework::crossproduct2<lz78::RefCharTuples, UniversalCodes, AllCodes>,
        framework::crossproduct2<lz78::RefCharArrays, UniversalCodes, AllCodes>
    >;
    using LZ78Tries = tl::list<lz78::BinaryTrie, lz78::BinaryTrieMTF>;

    framework::register_algorithms(framework::crossproduct2<LZ78, LZ78Codes, LZ78Tries>());
}
