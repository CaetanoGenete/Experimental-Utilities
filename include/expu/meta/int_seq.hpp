#ifndef SMM_INT_SEQ_HPP_INCLUDED
#define SMM_INT_SEQ_HPP_INCLUDED

#include <type_traits>

namespace expu {

    template<typename IntType, IntType... Seq>
    struct int_seq {
        static_assert(std::is_integral_v<IntType>, "IntType must be an integer!");

    public:
        using type = int_seq;

    public:
        static constexpr size_t size() { return sizeof...(Seq); }
    };

    template<typename, typename>
    struct _concat_shift_seq;

    template<typename IntType, IntType ... LhsSeq, IntType ... RhsSeq>
    struct _concat_shift_seq<int_seq<IntType, LhsSeq...>, int_seq<IntType, RhsSeq...>> :
        public int_seq<IntType, LhsSeq..., (RhsSeq + sizeof...(LhsSeq))...> {};


    template<typename IntType, size_t size>
    struct _make_int_seq_helper : public _concat_shift_seq<
        typename _make_int_seq_helper<IntType, size / 2>::type,
        typename _make_int_seq_helper<IntType, size - size / 2>::type> {};

    template<typename IntType>
    struct _make_int_seq_helper<IntType, 0> : public int_seq<IntType> {};

    template<typename IntType>
    struct _make_int_seq_helper<IntType, 1> : public int_seq<IntType, 0> {};

    template<typename IntType, size_t size>
    using make_int_seq = typename _make_int_seq_helper<IntType, size>::type;


    template<typename ... Seqs>
    struct _concat_int_seq;

    template<typename ... Seqs>
    using concat_int_seq = typename _concat_int_seq<Seqs...>::seq;

    template<typename IntType, IntType ... Seq>
    struct _concat_int_seq<int_seq<IntType, Seq...>> {
        using seq = int_seq<IntType, Seq...>;
    };

    template<typename First, typename Second, typename ... Seqs>
    struct _concat_int_seq<First, Second, Seqs...> {
        using seq = concat_int_seq<concat_int_seq<First, Second>, Seqs...>;
    };

    template<typename IntType, IntType ... LhsSeq, IntType ... RhsSeq>
    struct _concat_int_seq<int_seq<IntType, LhsSeq...>, int_seq<IntType, RhsSeq...>>
    {
        using seq = int_seq<IntType, LhsSeq..., RhsSeq...>;
    };

}

#endif // !SMM_INT_SEQ_INCLUDED_HPP
