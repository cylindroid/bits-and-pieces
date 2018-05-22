#include <cstdio>
#include <tuple>
#include <string>
#include <cstdint>

class Sequence
{
    public:
        using Id = uint64_t;
        Sequence() {}
};

class E : public Sequence
{
    public:
        static constexpr Id TypeId = 1;
        E() : _x(10), _y("abc") {}
    private:
        int _x;
        std::string _y;
};

class F : public Sequence
{
    public:
        F() : _z("def"), _u(5) {}
        static constexpr Id TypeId = 2;
    private:
        std::string _z;
        unsigned _u;
};


template <class One>
constexpr Sequence::Id SequenceIdAsFlag()
{
    static_assert(One::TypeId < (sizeof(uint64_t) * 8), "Out-of-bounds sequence Id - valid range: [0, 64)");
    return (1ull << One::TypeId);
}

template <class ... Elements>
constexpr Sequence::Id SequenceIdOr()
{
    std::initializer_list<Sequence::Id> list { SequenceIdAsFlag<Elements>()... };
    Sequence::Id ret = 0;
    for (Sequence::Id x : list)
    {
        ret |= x;
    }
    return ret;
}

template <class ... Elements>
class CompositeSequence
{
    public:
        CompositeSequence()
            : _elements()
        {}
        static constexpr uint64_t flag = SequenceIdOr<Elements...>();
    private:
        std::tuple<Elements...> _elements;
};

int main()
{
    CompositeSequence<E, F> cont;
    printf("%lu\n", CompositeSequence<E, F>::flag);
    return 0;
}
