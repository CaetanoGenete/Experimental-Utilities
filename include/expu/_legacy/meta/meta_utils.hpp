#ifndef EXPU_LEGACY_META_UTILS_HPP_INCLUDED
#define EXPU_LEGACY_META_UTILS_HPP_INCLUDED

#include "../../meta/meta_utils.hpp"

namespace expu::legacy {

    //LEGACY: log(n) instantiation depth cartesian product on std::tuple

    template<class ... Tuples>
    using tuple_concat_t = decltype(std::tuple_cat(std::declval<Tuples>()...));

    template<class Tuple>
    struct halve_tuple {
    private:
        static constexpr size_t _tuple_size = std::tuple_size_v<Tuple>;
        static constexpr size_t _half_size  = _tuple_size / 2;

    public:
        using lhs_type = tuple_subset_t<Tuple, std::make_index_sequence<_half_size>>;
        using rhs_type = tuple_subset_t<Tuple, make_index_sequence_from<_half_size, _tuple_size>>;

    };

    template<class LhsTuple, class RhsTuple>
    struct _cartesian_merge;

    template<class ... LhsTypes, class ... RhsTuples>
    struct _cartesian_merge<order_twople<LhsTypes...>, std::tuple<RhsTuples...>>
    {
        using type = std::tuple<tuple_concat_t<std::tuple<LhsTypes...>, RhsTuples>...>;
    };

    template<class ... LhsTuples, class ... RhsTuples>
    struct _cartesian_merge<std::tuple<LhsTuples...>, std::tuple<RhsTuples...>>
    {
        using type = tuple_concat_t<typename _cartesian_merge<std::tuple<LhsTuples>, std::tuple<RhsTuples...>>::type...>;
    };

    template<class Tuple>
    struct _cartesian_product;

    template<class ... Types>
    struct _cartesian_product<order_twople<Types...>>
    {
        using type = std::tuple<std::tuple<Types>...>;
    };

    template<class ... Tuples>
    struct _cartesian_product<std::tuple<Tuples...>>
    {
    private:
        using _halved_tuple = halve_tuple<std::tuple<Tuples...>>;

    public:
        //Divide and conquer
        using type =
            typename _cartesian_merge<
                typename _cartesian_product<typename _halved_tuple::lhs_type>::type,
                typename _cartesian_product<typename _halved_tuple::rhs_type>::type
            >::type;
    };

}

#endif // !EXPU_LEGACY_META_UTILS_HPP_INCLUDED