#include <cstdio>
#include <cassert>
#include <tuple>
#include <string>
#include <cstdint>
#include <memory>
#include <utility>


using SequenceIdType = uint32_t;
using CompositeSequenceTypeFlag = uint64_t;
using SequenceOpCode = uint32_t;

class Transaction
{
    public:
        Transaction() : _some_member(1) {}
    protected:
        int _some_member;
};

enum class SequenceId : SequenceIdType
{
    E,
    F,
    Max
};

using OpcodeBase = uint32_t;

enum class EOps : OpcodeBase
{
    RunA,
    RunB
};

enum class FOps : OpcodeBase
{
    RunX,
    RunY
};

template <SequenceId S, typename Opcode>
class Sequence
{
    public:
        using Id = SequenceId;
        using BaseType = Sequence<S, Opcode>;
        using OpcodeType = Opcode;
        static constexpr Id TypeId = S;
        Sequence(Transaction& trans) : _trans(trans) {}

        virtual void process(const Opcode op) = 0;
    protected:
        Transaction& _trans;
};

class E : public Sequence<SequenceId::E, EOps>
{
    public:
        E(Transaction& trans) : BaseType(trans), _x(10), _y("abc") {}
        virtual void process(const EOps op) override final
        {
            if (op == EOps::RunA)
            {
                fprintf(stderr, "E::RunA\n");
            } else
            {
                assert(EOps::RunB == op);
                fprintf(stderr, "E::RunB\n");
            }
        }
    private:
        int _x;
        std::string _y;
};

class F : public Sequence<SequenceId::F, FOps>
{
    public:
        F(Transaction& trans) : BaseType(trans), _z("def"), _u(5) {}

        virtual void process(const FOps op) override final
        {
            if (op == FOps::RunX)
            {
                fprintf(stderr, "F::RunX\n");
            } else
            {
                assert(FOps::RunY == op);
                fprintf(stderr, "F::RunY\n");
            }
        }
    private:
        std::string _z;
        unsigned _u;
};


template <class One>
constexpr CompositeSequenceTypeFlag SequenceIdAsFlag()
{
    static_assert(static_cast<SequenceIdType>(One::TypeId) < (sizeof(uint64_t) * 8), "Out-of-bounds sequence Id - valid range: [0, 64)");
    return (1ull << static_cast<SequenceIdType>(One::TypeId));
}

template <class ... Elements>
constexpr CompositeSequenceTypeFlag SequenceIdOr()
{
    std::initializer_list<CompositeSequenceTypeFlag> list { SequenceIdAsFlag<Elements>()... };
    CompositeSequenceTypeFlag ret = 0;
    for (CompositeSequenceTypeFlag x : list)
    {
        //static_assert((ret & x) == 0, "Template sequence arguments must not have duplicates");
        //static_assert(ret < x, "Template arguments of composite sequence must be in increasing sequence id order");
        ret |= x;
    }
    return ret;
}

template <class ... Elements>
constexpr size_t AggregateSizeInBytes()
{
    std::initializer_list<size_t> list { sizeof(Elements)... };
    size_t ret = 0;
    for (size_t x : list)
    {
        ret += x;
    }
    return ret;
}

template <class ... Elements>
class CompositeSequence
{
    public:
        using ComponentTuple = std::tuple<Elements...>;
        CompositeSequence(Transaction& trans)
            : _components(make_tuple((void(sizeof(Elements)), std::ref(trans))...))
        {}
        static constexpr CompositeSequenceTypeFlag composition_flag = SequenceIdOr<Elements...>();
        static constexpr uint32_t num_sequences = sizeof...(Elements);
        static bool isComponent(SequenceId seq_id)
        {
            return (composition_flag & (1ull << static_cast<SequenceIdType>(seq_id))) != 0;
        }

        template <typename Opcode>
        void run(SequenceId seq_id, const Opcode opcode)
        {
            if (isComponent(seq_id))
            {
                runImpl(seq_id, opcode, std::index_sequence_for<Elements...>{});
            }
        }
    private:
        template <typename Opcode, size_t ... Indexes>
        void runImpl(SequenceId seq_id, const Opcode opcode, std::index_sequence<Indexes...>)
        {
            std::initializer_list<int> list {
                ((seq_id == std::tuple_element<Indexes, ComponentTuple>::type::TypeId) ?
                    (std::get<Indexes>(_components).process(
                        static_cast<typename std::tuple_element<Indexes, ComponentTuple>::type::OpcodeType>(
                            opcode)), 0) : 0)...
            };
        }
        std::tuple<Elements...> _components;
};

int main()
{
    using TestSequence = CompositeSequence<E, F>;
    Transaction testtrans;
    TestSequence cont(testtrans);
    printf("composition flag: %lx\n", TestSequence::composition_flag);
    printf("number of sequences: %u\n", TestSequence::num_sequences);
    printf("size of component E: %lu\n", sizeof(E));
    printf("size of component F: %lu\n", sizeof(F));
    //printf("size of shared_ptr<E>: %lu\n", sizeof(std::shared_ptr<E>));
    //printf("size of shared_ptr<F>: %lu\n", sizeof(std::shared_ptr<F>));
    printf("size of Composite<E, F>: %lu\n", sizeof(TestSequence));
    cont.run(SequenceId::E, EOps::RunB);
    return 0;
}
